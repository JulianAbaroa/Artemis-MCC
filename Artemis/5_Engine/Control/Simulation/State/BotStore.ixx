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

        auto AddBot(const Bot& bot) -> void;
        auto GetBotCount() const -> std::size_t;
        auto CopyBots(std::vector<Bot>& out) const -> void;

        auto AddFakeMachine(const FakeMachine& machine) -> void;
        auto CopyFakeMachines(std::vector<FakeMachine>& out) const -> void;

        auto PublishSnapshot(const SessionSnapshot& snapshot) -> void;
        auto CopySnapshot(SessionSnapshot& out) const -> void;

        // Forgets the bots, their machines, the requests and the snapshot. The engine drops the players when the match ends.
        auto Cleanup() -> void;

    private:
        mutable std::mutex m_Mutex{};
        std::vector<std::optional<std::int8_t>> m_Requests{};
        std::vector<TeamRequest> m_TeamRequests{};
        std::vector<Bot> m_Bots{};
        std::vector<FakeMachine> m_FakeMachines{};
        SessionSnapshot m_Snapshot{};
    };
}