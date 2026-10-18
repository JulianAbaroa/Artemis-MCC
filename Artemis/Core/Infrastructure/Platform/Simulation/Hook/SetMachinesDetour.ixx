export module Platform.Simulation.Hook:SetMachines;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Simulation.Type;
import Platform.Simulation.State;
import std;

export namespace Platform::Simulation::Hook
{
    // Hooks players_set_machines so the machines of the bots survive every machine update of the engine (A9 M5b).
    // note: Without it, the next machine update from the session would drop the fake machines and the engine would remove their
    // players. Machines of the engine are never replaced: a fake machine whose slot is taken by a real one is skipped and logged.
    class SetMachinesDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using MachineIdentifier = Platform::Simulation::Type::MachineIdentifier;
        using SetMachinesFunction = Platform::Simulation::Type::SetMachinesFunction;

        using BotStore = Platform::Simulation::State::BotStore;

    public:
        SetMachinesDetour(LogsService& logsService, AOBService& aobService, BotStore& botStore) :
            m_LogsService(logsService), m_AOBService(aobService), m_BotStore(botStore) {}
        ~SetMachinesDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        BotStore& m_BotStore;

        static inline SetMachinesDetour* s_Instance{ nullptr };
        static inline SetMachinesFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        static auto __fastcall HookedSetMachines(std::uint32_t mask, const MachineIdentifier* identifiers) -> void;
    };
}