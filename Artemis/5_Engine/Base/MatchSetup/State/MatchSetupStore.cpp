module Engine.Base.MatchSetup.State;
import :MatchSetup;

namespace
{
    using Engine::Base::MatchSetup::Type::MatchSetupOutcome;
}

namespace Engine::Base::MatchSetup::State
{
    auto MatchSetupStore::Request(MatchSetup setup, MatchSetupOrigin origin) -> std::uint64_t
    {
        std::scoped_lock lock(m_Mutex);

        m_Pending = MatchSetupRequest{ .Value = std::move(setup), .Sequence = ++m_Sequence, .Origin = origin };
        m_Result = MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending, .Sequence = m_Sequence };
        m_HasPending.store(true, std::memory_order_release);
        return m_Sequence;
    }

    auto MatchSetupStore::Reject(std::string reason) -> std::uint64_t
    {
        std::scoped_lock lock(m_Mutex);
        m_Result = MatchSetupResult{ .Outcome = MatchSetupOutcome::Rejected, .Sequence = ++m_Sequence, .Reason = std::move(reason) };
        return m_Sequence;
    }

    auto MatchSetupStore::HasPending() const -> bool
    {
        return m_HasPending.load(std::memory_order_acquire);
    }

    auto MatchSetupStore::TakePending() -> std::optional<MatchSetupRequest>
    {
        std::scoped_lock lock(m_Mutex);
        m_HasPending.store(false, std::memory_order_release);
        return std::exchange(m_Pending, std::nullopt);
    }

    auto MatchSetupStore::TakePendingIf(const std::function<bool(const MatchSetup&)>& canServe) -> std::optional<MatchSetupRequest>
    {
        std::scoped_lock lock(m_Mutex);
        if (!m_Pending || !canServe(m_Pending->Value)) return std::nullopt;

        m_HasPending.store(false, std::memory_order_release);
        return std::exchange(m_Pending, std::nullopt);
    }

    auto MatchSetupStore::DropPending(MatchSetupOrigin origin) -> void
    {
        std::scoped_lock lock(m_Mutex);
        if (!m_Pending || m_Pending->Origin != origin) return;

        if (m_Result.Sequence == m_Pending->Sequence) m_Result = MatchSetupResult{};
        m_Pending.reset();
        m_HasPending.store(false, std::memory_order_release);
    }

    auto MatchSetupStore::GetSequence() const -> std::uint64_t
    {
        std::scoped_lock lock(m_Mutex);
        return m_Sequence;
    }

    auto MatchSetupStore::GetResult() const -> MatchSetupResult
    {
        std::scoped_lock lock(m_Mutex);
        return m_Result;
    }

    auto MatchSetupStore::Finish(const MatchSetupResult& result, const std::optional<Roster>& adoptedBots) -> void
    {
        std::scoped_lock lock(m_Mutex);

        if (adoptedBots) m_ActiveBots = adoptedBots;
        if (result.Sequence >= m_Result.Sequence) m_Result = result;
    }

    auto MatchSetupStore::GetActiveBots() const -> std::optional<Roster>
    {
        std::scoped_lock lock(m_Mutex);
        return m_ActiveBots;
    }

    auto MatchSetupStore::SetLoad(MatchSetupLoad load) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_Load = std::move(load);
    }

    auto MatchSetupStore::TakeLoad() -> std::optional<MatchSetupLoad>
    {
        std::scoped_lock lock(m_Mutex);
        return std::exchange(m_Load, std::nullopt);
    }

    auto MatchSetupStore::IsMatchStarted() const -> bool
    {
        std::scoped_lock lock(m_Mutex);
        return m_IsMatchStarted;
    }

    auto MatchSetupStore::SetMatchStarted() -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_IsMatchStarted = true;
    }

    auto MatchSetupStore::IsLoadWindow() const -> bool
    {
        std::scoped_lock lock(m_Mutex);
        return m_IsLoadWindow;
    }

    auto MatchSetupStore::SetLoadWindow(bool value) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_IsLoadWindow = value;
    }

    auto MatchSetupStore::Cleanup() -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_IsMatchStarted = false;
        m_IsLoadWindow = false;
        m_Load.reset();
        m_Result = m_Pending ? MatchSetupResult{ .Outcome = MatchSetupOutcome::Pending, .Sequence = m_Pending->Sequence } : MatchSetupResult{};
    }
}