module Platform.Lifecycle.State;
import :MatchCycle;

import std;

namespace Platform::Lifecycle::State
{
    auto MatchCycleStore::IsLobbyOpen() const -> bool
    {
        return m_IsLobbyOpen.load();
    }

    auto MatchCycleStore::SetLobbyOpen(bool value) -> void
    {
        m_IsLobbyOpen.store(value);
    }

    auto MatchCycleStore::AddEngineStart() -> void
    {
        m_EngineStarts.fetch_add(1);
    }

    auto MatchCycleStore::GetEngineStarts() const -> std::uint64_t
    {
        return m_EngineStarts.load();
    }

    auto MatchCycleStore::AddEngineDestroy() -> void
    {
        m_EngineDestroys.fetch_add(1);
    }

    auto MatchCycleStore::GetEngineDestroys() const -> std::uint64_t
    {
        return m_EngineDestroys.load();
    }

    auto MatchCycleStore::SetEngineGlobal(void** global) -> void
    {
        m_EngineGlobal.store(global);
    }

    auto MatchCycleStore::GetEngineGlobal() const -> void**
    {
        return m_EngineGlobal.load();
    }

    auto MatchCycleStore::RequestLeave() -> void
    {
        m_IsLeaveRequested.store(true);
    }

    auto MatchCycleStore::TakeLeaveRequest() -> bool
    {
        return m_IsLeaveRequested.exchange(false);
    }

    auto MatchCycleStore::GetLastLeave() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_LastLeave;
    }

    auto MatchCycleStore::SetLastLeave(std::string result) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_LastLeave = std::move(result);
    }
}