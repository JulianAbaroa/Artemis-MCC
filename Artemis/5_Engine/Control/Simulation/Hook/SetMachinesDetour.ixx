export module Engine.Control.Simulation.Hook:SetMachines;

import Common.Log.System;
import Engine.Base.Memory.System;
import Engine.Control.Simulation.Type;
import Engine.Control.Simulation.State;
import std;

export namespace Engine::Control::Simulation::Hook
{
    // Hooks players_set_machines so the machines of the bots survive every machine update of the engine (A9 M5b).
    // note: Without it, the next machine update from the session would drop the fake machines and the engine would remove their
    // players. Machines of the engine are never replaced: a fake machine whose slot is taken by a real one is skipped and logged.
    class SetMachinesDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Engine::Base::Memory::System::AOBService;

        using MachineIdentifier = Engine::Control::Simulation::Type::MachineIdentifier;
        using SetMachinesFunction = Engine::Control::Simulation::Type::SetMachinesFunction;

        using BotStore = Engine::Control::Simulation::State::BotStore;

    public:
        SetMachinesDetour(Log& log, AOBService& aobService, BotStore& botStore) :
            m_Log(log), m_AOBService(aobService), m_BotStore(botStore) {}
        ~SetMachinesDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
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