module Platform.Match.State;
import :MatchConfig;

import Platform.Match.Type;
import std;

namespace Platform::Match::State
{
    auto MatchConfigStore::GetPending() const -> std::shared_ptr<const MatchConfig>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Pending;
    }

    auto MatchConfigStore::SetPending(std::shared_ptr<const MatchConfig> config) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Pending = std::move(config);
    }

    auto MatchConfigStore::TakePending() -> std::shared_ptr<const MatchConfig>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return std::exchange(m_Pending, nullptr);
    }

    auto MatchConfigStore::GetRecordName() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_RecordName;
    }

    auto MatchConfigStore::SetRecordName(std::string name) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_RecordName = std::move(name);
    }

    auto MatchConfigStore::TakeRecordName() -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return std::exchange(m_RecordName, std::string{});
    }

    auto MatchConfigStore::GetLastApplied() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_LastApplied;
    }

    auto MatchConfigStore::SetLastApplied(std::string name) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_LastApplied = std::move(name);
    }

    auto MatchConfigStore::GetLastRecorded() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_LastRecorded;
    }

    auto MatchConfigStore::SetLastRecorded(std::string file) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_LastRecorded = std::move(file);
    }

    auto MatchConfigStore::IsLaunchRequested() const -> bool
    {
        return m_IsLaunchRequested.load();
    }

    auto MatchConfigStore::SetLaunchRequested(bool value) -> void
    {
        if (value)
        {
            std::lock_guard<std::mutex> lock{ m_Mutex };
            m_LaunchRequestTime = steady_clock::now();
        }
        m_IsLaunchRequested.store(value);
    }

    auto MatchConfigStore::GetLaunchRequestTime() const -> steady_clock::time_point
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_LaunchRequestTime;
    }

    auto MatchConfigStore::TakeLaunchRequest() -> bool
    {
        return m_IsLaunchRequested.exchange(false);
    }

    auto MatchConfigStore::GetLastLaunch() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_LastLaunch;
    }

    auto MatchConfigStore::SetLastLaunch(std::string result) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_LastLaunch = std::move(result);
    }
}