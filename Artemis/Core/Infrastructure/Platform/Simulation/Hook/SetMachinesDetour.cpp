module Platform.Simulation.Hook;
import :SetMachines;

import Platform.Memory.Type;
import Platform.Hook.System;
import std;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Simulation::Type::MachineIdentifier;
    using Platform::Simulation::Type::FakeMachine;

    using Platform::Simulation::Type::k_MaxMachines;
}

namespace Platform::Simulation::Hook
{
    auto __fastcall SetMachinesDetour::HookedSetMachines(std::uint32_t mask, const MachineIdentifier* identifiers) -> void
    {
        Platform::Hook::System::InFlightScope scope(s_InFlight);

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
                s_Instance->m_LogsService.Message("[SetMachinesDetour] WARNING: A real machine took slot {}. Its bots will be dropped.",
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

        void* functionAddress{ reinterpret_cast<void*>(m_AOBService.FindPattern(Signature::SetMachines)) };
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedSetMachines),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[SetMachinesDetour]", m_LogsService))
        {
            return false;
        }

        m_IsHookInstalled.store(true);
        return true;
    }

    auto SetMachinesDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        Platform::Hook::System::WaitForDrain(s_InFlight, std::chrono::milliseconds(500));
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[SetMachinesDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}