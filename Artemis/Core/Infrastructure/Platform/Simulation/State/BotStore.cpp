module Platform.Simulation.State;

namespace Platform::Simulation::State
{
    auto BotStore::RequestBot() -> void
    {
        std::scoped_lock lock(m_Mutex);
        ++m_Requests;
    }

    auto BotStore::TakeRequests() -> std::size_t
    {
        std::scoped_lock lock(m_Mutex);
        return std::exchange(m_Requests, 0);
    }

    auto BotStore::AddBot(const Bot& bot) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_Bots.push_back(bot);
    }

    auto BotStore::GetBotCount() const -> std::size_t
    {
        std::scoped_lock lock(m_Mutex);
        return m_Bots.size();
    }

    auto BotStore::CopyBots(std::vector<Bot>& out) const -> void
    {
        std::scoped_lock lock(m_Mutex);
        out = m_Bots;
    }

    auto BotStore::AddFakeMachine(const FakeMachine& machine) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_FakeMachines.push_back(machine);
    }

    auto BotStore::CopyFakeMachines(std::vector<FakeMachine>& out) const -> void
    {
        std::scoped_lock lock(m_Mutex);
        out = m_FakeMachines;
    }

    auto BotStore::PublishSnapshot(const SessionSnapshot& snapshot) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_Snapshot = snapshot;
    }

    auto BotStore::CopySnapshot(SessionSnapshot& out) const -> void
    {
        std::scoped_lock lock(m_Mutex);
        out = m_Snapshot;
    }

    auto BotStore::Cleanup() -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_Requests = 0;
        m_Bots.clear();
        m_FakeMachines.clear();
        m_Snapshot = SessionSnapshot{};
    }
}