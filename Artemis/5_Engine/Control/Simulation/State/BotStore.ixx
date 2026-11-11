export module Engine.Control.Simulation.State;

import Engine.Control.Simulation.Type;
import std;

export namespace Engine::Control::Simulation::State
{
    // Bots Artemis adds to the match (players on machines of their own), the machines they use, and the latest session snapshot.
    // note: Thread-safe. The UI asks for bots, the game thread adds them and publishes the snapshot once per tick (BotService).
    class BotStore
    {
    private:
        using SessionSnapshot = Engine::Control::Simulation::Type::SessionSnapshot;
        using Bot = Engine::Control::Simulation::Type::Bot;
        using FakeMachine = Engine::Control::Simulation::Type::FakeMachine;
        using TeamRequest = Engine::Control::Simulation::Type::TeamRequest;
        using Roster = Engine::Control::Simulation::Type::Roster;
        using RosterRequest = Engine::Control::Simulation::Type::RosterRequest;
        using RosterResult = Engine::Control::Simulation::Type::RosterResult;

    public:
        BotStore() = default;
        ~BotStore() = default;

        BotStore(const BotStore&) = delete;
        auto operator=(const BotStore&) -> BotStore& = delete;

        // Asks for one more bot. The game thread adds it on its next tick.
        // param team: Team of the bot in team games, or nullopt for the team of the local player.
        auto RequestBot(std::optional<std::int8_t> team) -> void;

        // return: The teams of the bots asked for and not added yet (one entry per bot), and clears them.
        auto TakeRequests() -> std::vector<std::optional<std::int8_t>>;

        // Asks to move a bot to another team. The game thread applies it on its next tick.
        auto RequestTeam(const TeamRequest& request) -> void;

        // return: The team changes asked for and not applied yet, and clears them.
        auto TakeTeamRequests() -> std::vector<TeamRequest>;

        // Asks to remove a bot by its number. The game thread removes it on its next tick.
        auto RequestRemove(std::size_t number) -> void;

        // return: The numbers of the bots asked to be removed and not removed yet, and clears them.
        auto TakeRemoveRequests() -> std::vector<std::size_t>;

        // Asks for a roster of bots (D284). Before the first tick of a match it is the roster of that load. In a match it waits for the
        // end of the round. A newer request replaces an older one that was not taken, and the result goes to Pending.
        auto RequestRoster(const Roster& roster) -> void;

        auto HasPendingRoster() const -> bool;

        // return: The roster asked for and not applied yet, and clears it. The result stays Pending until FinishRoster.
        auto TakePendingRoster() -> std::optional<RosterRequest>;

        // return: What the first tick of a match applies: the pending request, which is taken, or else the active roster (with the number
        // of the latest request). Nothing if there is neither.
        auto TakeLoadRoster() -> std::optional<RosterRequest>;

        // Records what happened to a request, in one step under the lock.
        // param isApplied: The plan was applied: the roster becomes the active one, which the first tick of every match applies.
        // note: The result is only replaced if no newer request was made since, so an old apply never hides a newer Pending. A request that
        // was applied becomes the active roster even then, because it is what the engine holds.
        auto FinishRoster(const RosterRequest& request, const RosterResult& result, bool isApplied) -> void;

        auto GetActiveRoster() const -> std::optional<Roster>;

        // Forgets the active roster, the pending one and the result.
        auto ClearRoster() -> void;

        auto GetRosterResult() const -> RosterResult;

        // Whether the first tick of the match went by (the roster of the load was applied). Cleanup resets it.
        auto IsMatchStarted() const -> bool;
        auto SetMatchStarted() -> void;

        auto AddBot(const Bot& bot) -> void;

        // Forgets the bot with that number. Its machine stays.
        auto RemoveBot(std::size_t number) -> void;
        auto CopyBots(std::vector<Bot>& out) const -> void;

        auto AddFakeMachine(const FakeMachine& machine) -> void;
        auto CopyFakeMachines(std::vector<FakeMachine>& out) const -> void;

        auto PublishSnapshot(const SessionSnapshot& snapshot) -> void;
        auto CopySnapshot(SessionSnapshot& out) const -> void;

        // Forgets the bots, their machines, the requests and the snapshot. The engine drops the players when the match ends.
        // note: The active roster and a pending one stay, like the preset: the next match applies them at its first tick. The result goes
        // back to neutral (Pending if a request waits), so the lobby does not show what happened in the match.
        auto Cleanup() -> void;

    private:
        mutable std::mutex m_Mutex{};
        std::vector<std::optional<std::int8_t>> m_Requests{};
        std::vector<TeamRequest> m_TeamRequests{};
        std::vector<std::size_t> m_RemoveRequests{};
        std::optional<Roster> m_ActiveRoster{};
        std::optional<RosterRequest> m_PendingRoster{};
        std::uint64_t m_RosterSequence{ 0 };
        RosterResult m_RosterResult{};
        bool m_IsMatchStarted{ false };
        std::vector<Bot> m_Bots{};
        std::vector<FakeMachine> m_FakeMachines{};
        SessionSnapshot m_Snapshot{};
    };
}