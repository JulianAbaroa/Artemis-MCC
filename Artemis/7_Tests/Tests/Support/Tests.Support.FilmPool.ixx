module;

#include <gtest/gtest.h>

#include <stdlib.h>

export module Tests.Support.FilmPool;

import Tests.Support.FilmPass;
import std;

// The passes of the process (D269): each pass over a film runs once, on a small pool of threads that takes the biggest first, and the tests
// wait for the result they need. The passes of every suite share the pool, so the cap of simultaneous passes holds for all of them.

namespace
{
    using Tests::Support::FilmPass::PeakWorkingSetMegabytes;
    using Tests::Support::FilmPass::RecordingsFolder;

    constexpr std::string_view k_JobsVariable{ "ARTEMIS_REPLAY_JOBS" };

    // note: More passes at once share memory bandwidth and cache: on the 10-core laptop (RB.1, 2026-11-23) ReplayTests took 224 s with
    // the 16 hardware threads, 147 s with 6, 118 s with 4 and 132 s with 3.
    constexpr std::size_t k_DefaultReplayJobs{ 4 };

    // return: How many passes may run at once: ARTEMIS_REPLAY_JOBS if it is set, else k_DefaultReplayJobs.
    auto ReplayJobs() -> std::size_t
    {
        char* value{ nullptr };
        std::size_t length{};
        std::size_t jobs{ 0 };

        if (_dupenv_s(&value, &length, k_JobsVariable.data()) == 0 && value != nullptr)
        {
            jobs = static_cast<std::size_t>(std::strtoul(value, nullptr, 10));
            std::free(value);
        }

        if (jobs == 0) jobs = k_DefaultReplayJobs;
        return (std::max)(jobs, std::size_t{ 1 });
    }
}

export namespace Tests::Support::FilmPool
{
    // A pass to run in the pool. Run gives the result that Get returns for the key (any copyable type).
    struct PoolTask
    {
        std::string Key{};
        std::uintmax_t Size{};
        std::function<std::any()> Run{};
    };

    // return: The task of a pass that gives a T.
    // param size: Cost of the pass, the biggest start first (SizeOfFilm for a pass over a film).
    template <typename T>
    auto MakeTask(std::string key, std::uintmax_t size, std::function<T()> run) -> PoolTask
    {
        return PoolTask{ .Key = std::move(key), .Size = size, .Run = [run = std::move(run)] { return std::any{ run() }; } };
    }

    // return: The size of the golden.sdump of the film (0 if it has none).
    inline auto SizeOfFilm(const std::string& film) -> std::uintmax_t
    {
        std::error_code error{};
        const std::uintmax_t size{ std::filesystem::file_size(RecordingsFolder() / film / "golden.sdump", error) };
        return error ? 0 : size;
    }

    // return: Whether the filter of the run selects the test (it is registered and not filtered out).
    inline auto IsTestSelected(std::string_view suiteName, std::string_view testName) -> bool
    {
        const ::testing::UnitTest& unit{ *::testing::UnitTest::GetInstance() };
        for (int suiteIndex{ 0 }; suiteIndex < unit.total_test_suite_count(); ++suiteIndex)
        {
            const ::testing::TestSuite& suite{ *unit.GetTestSuite(suiteIndex) };
            if (std::string_view{ suite.name() } != suiteName) continue;

            for (int testIndex{ 0 }; testIndex < suite.total_test_count(); ++testIndex)
            {
                const ::testing::TestInfo& info{ *suite.GetTestInfo(testIndex) };
                if (info.should_run() && std::string_view{ info.name() } == testName) return true;
            }
        }

        return false;
    }

    class PassPool
    {
    public:
        PassPool() = default;
        ~PassPool() = default;
        PassPool(const PassPool&) = delete;
        auto operator=(const PassPool&) -> PassPool& = delete;

        // Adds a source of passes: Start asks it for the tasks of the tests the filter selected.
        // note: Call it before the tests run (from the initialization of the file that owns the passes).
        auto Register(std::function<std::vector<PoolTask>()> provider) -> bool
        {
            const std::scoped_lock lock{ m_Mutex };
            m_Providers.push_back(std::move(provider));
            return true;
        }

        // Queues the passes of every provider that were not queued yet, so they run at once while the first tests wait.
        // note: Every suite that waits for a pass calls it in SetUpTestSuite: the first call starts the passes of all of them.
        auto Start() -> void
        {
            std::vector<std::function<std::vector<PoolTask>()>> providers{};
            {
                const std::scoped_lock lock{ m_Mutex };
                providers = m_Providers;
            }

            std::vector<PoolTask> tasks{};
            for (const auto& provider : providers)
            {
                for (PoolTask& task : provider()) tasks.push_back(std::move(task));
            }

            this->Launch(std::move(tasks));
        }

        // return: The result of the pass, waiting for it to end. The pass runs on this thread if nobody queued it.
        // note: An exception that escapes the pass is thrown again here.
        template <typename T>
        auto Get(const std::string& key, const std::function<T()>& run) -> const T&
        {
            return std::any_cast<const T&>(this->GetAny(key, [&run] { return std::any{ run() }; }));
        }

        // Prints a line of the passes without mixing it with the line of another thread.
        auto Print(const std::string& line) -> void
        {
            const std::scoped_lock lock{ m_Output };
            std::cout << line << std::flush;
        }

        // Waits for the passes that are still running.
        auto Join() -> void
        {
            m_Workers.clear();
        }

    private:
        struct Pending
        {
            std::string Key{};
            std::uintmax_t Size{};
            std::function<std::any()> Run{};
            std::promise<std::any> Promise{};
        };

        auto Launch(std::vector<PoolTask> tasks) -> void
        {
            const std::size_t cap{ ReplayJobs() };
            const std::scoped_lock lock{ m_Mutex };

            for (PoolTask& task : tasks)
            {
                if (m_Results.contains(task.Key)) continue;

                Pending& pending{ m_Queue.emplace_back() };
                pending.Key = task.Key;
                pending.Size = task.Size;
                pending.Run = std::move(task.Run);
                m_Results.emplace(task.Key, pending.Promise.get_future().share());
            }

            // The biggest first: the suite takes as long as the longest pass if it starts at once.
            std::ranges::stable_sort(m_Queue, [](const Pending& a, const Pending& b) { return a.Size > b.Size; });

            const std::size_t free{ cap > m_Active ? cap - m_Active : 0 };
            const std::size_t spawn{ (std::min)(free, m_Queue.size()) };
            for (std::size_t i{ 0 }; i < spawn; ++i)
            {
                ++m_Active;
                m_Workers.emplace_back([this] { this->Work(); });
            }
        }

        auto GetAny(const std::string& key, const std::function<std::any()>& run) -> const std::any&
        {
            std::shared_future<std::any> future{};
            std::promise<std::any> promise{};
            bool isOwner{ false };
            {
                const std::scoped_lock lock{ m_Mutex };
                if (const auto it{ m_Results.find(key) }; it != m_Results.end())
                {
                    future = it->second;
                }
                else
                {
                    future = promise.get_future().share();
                    m_Results.emplace(key, future);
                    isOwner = true;
                }
            }

            if (isOwner) this->Fulfill(promise, run);
            return future.get();
        }

        static auto Fulfill(std::promise<std::any>& promise, const std::function<std::any()>& run) -> void
        {
            try
            {
                promise.set_value(run());
            }
            catch (...)
            {
                promise.set_exception(std::current_exception());
            }
        }

        auto Work() -> void
        {
            bool isLast{ false };
            for (;;)
            {
                Pending pending{};
                {
                    const std::scoped_lock lock{ m_Mutex };
                    if (m_Queue.empty())
                    {
                        isLast = --m_Active == 0;
                        break;
                    }

                    pending = std::move(m_Queue.front());
                    m_Queue.pop_front();
                }

                Fulfill(pending.Promise, pending.Run);
            }

            if (isLast)
            {
                this->Print(std::format("[ REPLAY MEM ] peak working set {} MB, {} jobs, hardware_concurrency {}\n", PeakWorkingSetMegabytes(), ReplayJobs(),
                    std::thread::hardware_concurrency()));
            }
        }

        std::mutex m_Mutex{};
        std::mutex m_Output{};
        std::vector<std::function<std::vector<PoolTask>()>> m_Providers{};
        std::deque<Pending> m_Queue{};
        std::map<std::string, std::shared_future<std::any>> m_Results{};
        std::vector<std::jthread> m_Workers{};
        std::size_t m_Active{ 0 };
    };

    // return: The pool of the process, shared by the suites that wait for passes over the films.
    // note: One pool for all: the cap of simultaneous passes holds only if every suite queues in the same one.
    inline auto SharedPool() -> PassPool&
    {
        static PassPool pool{};
        return pool;
    }
}