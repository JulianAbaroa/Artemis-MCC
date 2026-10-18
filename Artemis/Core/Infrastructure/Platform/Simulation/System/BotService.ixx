export module Platform.Simulation.System;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Simulation.Type;
import Platform.Simulation.State;
import std;

export namespace Platform::Simulation::System
{
    // Adds bots to the match: each one is a player of a machine that only Artemis knows, created like a player that joins in progress
    // (A9 M5b). Also publishes the machines and players of the engine every tick (A9 M5a).
    // note: The simulation sees a bot as a remote player: no view, no local input. Its action of each tick comes from the overrides of
    // the PlayerAction hook (D109). The bots skip the session and the simulation queue, so the film does not have them (A9 sec. 13.4).
    class BotService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using SetMachinesFunction = Platform::Simulation::Type::SetMachinesFunction;
        using ApplyPlayerAddedFunction = Platform::Simulation::Type::ApplyPlayerAddedFunction;

        using BotStore = Platform::Simulation::State::BotStore;

    public:
        BotService(LogsService& logsService, AOBService& aobService, BotStore& botStore) :
            m_LogsService(logsService), m_AOBService(aobService), m_BotStore(botStore) {}
        ~BotService() = default;

        BotService(const BotService&) = delete;
        auto operator=(const BotService&) -> BotService& = delete;

        // Publishes the session snapshot and adds the bots asked for since the last tick.
        // param threadContext: Thread context of the engine, or 0 if unknown (then it does nothing).
        // note: Game thread only, before the player actions of the tick are applied.
        auto Update(std::uintptr_t threadContext) -> void;

        // Finds the engine functions by signature.
        // return: False if one is not found. The cause is logged.
        // note: Call it before the SetMachines hook is installed: the hook overwrites the bytes the signature matches.
        auto ResolveFunctions() -> bool;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        BotStore& m_BotStore;

        // Ticks seen. Game thread only.
        std::uint64_t m_Sequence{ 0 };

        // Engine functions. The SetMachines call goes through its hook, which keeps the fake machines.
        SetMachinesFunction m_SetMachines{ nullptr };
        ApplyPlayerAddedFunction m_ApplyPlayerAdded{ nullptr };
        bool m_IsResolved{ false };

        // Reads the machines, the players and the game options of the engine into the store.
        auto PublishSnapshot(std::uintptr_t threadContext) -> void;

        // Creates the machine of the next bot if needed, then the bot.
        // return: False if the engine refused it. The cause is logged.
        auto AddBot(std::uintptr_t threadContext) -> bool;

        // Makes sure the fake machine exists in the engine.
        // return: False if the slot belongs to a real machine or the engine did not take it.
        auto EnsureFakeMachine(std::uintptr_t playersGlobals, std::size_t machineIndex) -> bool;
    };
}