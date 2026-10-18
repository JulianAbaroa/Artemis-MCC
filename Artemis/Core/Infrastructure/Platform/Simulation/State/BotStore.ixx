export module Platform.Simulation.State;

import Platform.Simulation.Type;
import std;

export namespace Platform::Simulation::State
{
    // Bots Artemis adds to the match (players on machines of their own), the machines they use, and the latest session snapshot.
    // note: Thread-safe. The UI asks for bots, the game thread adds them and publishes the snapshot once per tick (BotService).
    class BotStore
    {
    private:
        using SessionSnapshot = Platform::Simulation::Type::SessionSnapshot;
        using Bot = Platform::Simulation::Type::Bot;
        using FakeMachine = Platform::Simulation::Type::FakeMachine;

    public:
        BotStore() = default;
        ~BotStore() = default;

        BotStore(const BotStore&) = delete;
        auto operator=(const BotStore&) -> BotStore& = delete;

        // Asks for one more bot. The game thread adds it on its next tick.
        auto RequestBot() -> void;

        // return: The bots asked for and not added yet, and clears them.
        auto TakeRequests() -> std::size_t;

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
        std::size_t m_Requests{ 0 };
        std::vector<Bot> m_Bots{};
        std::vector<FakeMachine> m_FakeMachines{};
        SessionSnapshot m_Snapshot{};
    };
}