module Engine.Control.Simulation.Hook;
import :SetMachines;

import Engine.Base.Memory.Type;
import Engine.Base.Hook.System;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using Engine::Base::Hook::System::k_DrainTimeout;

    using Engine::Control::Simulation::Type::MachineIdentifier;
    using Engine::Control::Simulation::Type::FakeMachine;

    using Engine::Control::Simulation::Type::k_MaxMachines;
}

namespace Engine::Control::Simulation::Hook
{
    auto __fastcall SetMachinesDetour::HookedSetMachines(std::uint32_t mask, const MachineIdentifier* identifiers) -> void
    {
        Engine::Base::Hook::System::InFlightScope scope(s_InFlight);

        std::vector<FakeMachine> fakeMachines{};
        s_Instance->m_BotStore.CopyFakeMachines(fakeMachines);

        if (fakeMachines.empty() || identifiers == nullptr)
        {
            s_OriginalFunction(mask, identifiers);
            return;
        }

        std::array<MachineIdentifier, k_MaxMachines> merged{};
        std::copy_n(identifiers, k_MaxMachines, merged.begin());

        for (const FakeMachine& machine : fakeMachines)
        {
            const std::uint32_t bit{ 1u << machine.Index };

            if ((mask & bit) != 0 && merged[machine.Index].Bytes != machine.Identifier.Bytes)
            {
                s_Instance->m_Log.Message("[SetMachinesDetour] WARNING: A real machine took slot {}. Its bots will be dropped.",
                    machine.Index);
                continue;
            }

            mask |= bit;
            merged[machine.Index] = machine.Identifier;
        }

        s_OriginalFunction(mask, merged.data());
    }

    auto SetMachinesDetour::Install() -> bool
    {
        if (m_IsHookInstalled.load()) return true;
        s_Instance = this;

        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindPattern(Signature::k_SetMachines)) };
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedSetMachines),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[SetMachinesDetour]", m_Log))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto SetMachinesDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout);
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[SetMachinesDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}