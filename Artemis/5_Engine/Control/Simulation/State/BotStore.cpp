module Engine.Control.Simulation.State;

namespace
{
    using Engine::Control::Simulation::Type::RosterOutcome;
}

namespace Engine::Control::Simulation::State
{
    auto BotStore::RequestBot(std::optional<std::int8_t> team) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_Requests.push_back(team);
    }

    auto BotStore::TakeRequests() -> std::vector<std::optional<std::int8_t>>
    {
        std::scoped_lock lock(m_Mutex);
        return std::exchange(m_Requests, {});
    }

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

    auto BotStore::RequestRoster(const Roster& roster) -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_PendingRoster = RosterRequest{ .Value = roster, .Sequence = ++m_RosterSequence };
        m_RosterResult = RosterResult{ .Outcome = RosterOutcome::Pending };
    }

    auto BotStore::HasPendingRoster() const -> bool
    {
        std::scoped_lock lock(m_Mutex);
        return m_PendingRoster.has_value();
    }

    auto BotStore::TakePendingRoster() -> std::optional<RosterRequest>
    {
        std::scoped_lock lock(m_Mutex);
        return std::exchange(m_PendingRoster, std::nullopt);
    }

    auto BotStore::TakeLoadRoster() -> std::optional<RosterRequest>
    {
        std::scoped_lock lock(m_Mutex);

        if (m_PendingRoster) return std::exchange(m_PendingRoster, std::nullopt);
        if (m_ActiveRoster) return RosterRequest{ .Value = *m_ActiveRoster, .Sequence = m_RosterSequence };

        return std::nullopt;
    }

    auto BotStore::FinishRoster(const RosterRequest& request, const RosterResult& result, bool isApplied) -> void
    {
        std::scoped_lock lock(m_Mutex);

        if (isApplied) m_ActiveRoster = request.Value;
        if (request.Sequence == m_RosterSequence) m_RosterResult = result;
    }

    auto BotStore::GetActiveRoster() const -> std::optional<Roster>
    {
        std::scoped_lock lock(m_Mutex);
        return m_ActiveRoster;
    }

    auto BotStore::ClearRoster() -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_ActiveRoster.reset();
        m_PendingRoster.reset();
        m_RosterResult = RosterResult{};
    }

    auto BotStore::GetRosterResult() const -> RosterResult
    {
        std::scoped_lock lock(m_Mutex);
        return m_RosterResult;
    }

    auto BotStore::IsMatchStarted() const -> bool
    {
        std::scoped_lock lock(m_Mutex);
        return m_IsMatchStarted;
    }

    auto BotStore::SetMatchStarted() -> void
    {
        std::scoped_lock lock(m_Mutex);
        m_IsMatchStarted = true;
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
        m_Requests.clear();
        m_TeamRequests.clear();
        m_RemoveRequests.clear();
        m_IsMatchStarted = false;
        m_RosterResult = m_PendingRoster ? RosterResult{ .Outcome = RosterOutcome::Pending } : RosterResult{};
        m_Bots.clear();
        m_FakeMachines.clear();
        m_Snapshot = SessionSnapshot{};
    }
}