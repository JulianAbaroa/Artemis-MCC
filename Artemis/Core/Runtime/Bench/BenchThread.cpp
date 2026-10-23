module Runtime.Thread;
import :Bench;

import std;

namespace
{
    using std::chrono::milliseconds;

    // Time between two updates of the bench.
    constexpr milliseconds k_UpdateInterval{ 200 };
}

namespace Runtime::Thread
{
    auto BenchThread::Run() -> void
    {
        auto& lifecycle{ m_Platform.m_LifecycleStore };

        m_BenchUIService.Initialize();
        m_Service.m_Log.Message("[BenchThread] INFO: Started.");

        std::unique_lock<std::mutex> lock{ lifecycle.GetShutdownMutex() };

        while (lifecycle.IsRunning())
        {
            lock.unlock();

            // A failure of the bench (a file it cannot write, a scenario it cannot read) must not take the game down.
            try
            {
                m_BenchUIService.Update();
            }
            catch (const std::exception& exception)
            {
                m_Service.m_Log.Message("[BenchThread] ERROR: Bench update failed ({}).", exception.what());
            }

            lock.lock();
            lifecycle.GetShutdownCV().wait_for(lock, k_UpdateInterval, [&] {
                return !lifecycle.IsRunning();
            });
        }

        lock.unlock();
        m_Service.m_Log.Message("[BenchThread] INFO: Stopped.");
    }
}