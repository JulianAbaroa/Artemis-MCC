module Engine.Control.Simulation.State;

namespace Engine::Control::Simulation::State
{
    auto BotStore::RequestTeam(const TeamRequest& request) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_TeamRequests.push_back(request);
    }

    auto BotStore::TakeTeamRequests() -> std::vector<TeamRequest>
    {
        std::scoped_lock lock(m_Mutex);
        return std::exchange(m_TeamRequests, {});
    }

    auto BotStore::RequestRemove(std::size_t number) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_RemoveRequests.push_back(number);
    }

    auto BotStore::TakeRemoveRequests() -> std::vector<std::size_t>
    {
        std::scoped_lock lock(m_Mutex);
        return std::exchange(m_RemoveRequests, {});
    }

    auto BotStore::AddBot(const Bot& bot) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_Bots.push_back(bot);
    }

    auto BotStore::RemoveBot(std::size_t number) -> void
    {
        std::scoped_lock lock(m_Mutex);
        std::erase_if(m_Bots, [number](const Bot& bot) { return bot.Number == number; });
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
        m_TeamRequests.clear();
        m_RemoveRequests.clear();
        m_Bots.clear();
        m_FakeMachines.clear();
        m_Snapshot = SessionSnapshot{};
    }
}