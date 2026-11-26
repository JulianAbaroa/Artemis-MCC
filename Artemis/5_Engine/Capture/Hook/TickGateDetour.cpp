module;

#include <intrin.h>
#include "External/minhook/include/MinHook.h"

module Engine.Capture.Hook;
import :TickGate;

import Engine.Base.Memory.Type;
import Engine.Base.Hook.System;
import std;

namespace
{
    namespace Signature = Engine::Base::Memory::Type::Signature;

    using Engine::Base::Hook::System::k_DrainTimeout;
    using Signature::k_CallOpcode;
    using Signature::k_PauseCheckPrologue;
    using Signature::k_SimulationTicksPauseCallOffset;
    using Signature::k_SimulationTicksPauseCallSize;
}

namespace Engine::Capture::Hook
{
    auto __fastcall TickGateDetour::HookedPauseCheck() -> bool
    {
        Engine::Base::Hook::System::InFlightScope scope{ s_InFlight };

        // The engine answer first: a pause or a menu already ends the loop.
        if (s_OriginalFunction()) return true;

        auto* self{ s_Instance };
        if (reinterpret_cast<std::uintptr_t>(_ReturnAddress()) != s_LoopReturn || self->m_TickGate == nullptr) return false;

        return self->m_TickGate->IsTickHeld();
    }

    auto TickGateDetour::SetTickGate(TickGate& tickGate) -> void
    {
        m_TickGate = &tickGate;
    }

    auto TickGateDetour::Install(void* simulationTicksAddress, bool isTickCounted) -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        constexpr const char* k_Consequence{ " The game freezes while it waits for Python, as before TG.1." };

        const auto simulationTicks{ reinterpret_cast<std::uintptr_t>(simulationTicksAddress) };
        if (simulationTicks == 0)
        {
            m_Log.Message("[TickGateDetour] ERROR: SimulationTicks is not hooked.{}", k_Consequence);
            return;
        }

        if (!isTickCounted)
        {
            m_Log.Message("[TickGateDetour] ERROR: ExecuteGameFrame is not hooked, a held frame would produce a tick again.{}", k_Consequence);
            return;
        }

        const auto* call{ reinterpret_cast<const std::uint8_t*>(simulationTicks + k_SimulationTicksPauseCallOffset) };
        if (call[0] != k_CallOpcode)
        {
            m_Log.Message("[TickGateDetour] ERROR: No call at the pause check of SimulationTicks.{}", k_Consequence);
            return;
        }

        std::int32_t rel32{ 0 };
        std::memcpy(&rel32, call + 1, sizeof(rel32));
        const std::uintptr_t loopReturn{ simulationTicks + k_SimulationTicksPauseCallOffset + k_SimulationTicksPauseCallSize };
        const std::uintptr_t target{ loopReturn + static_cast<std::intptr_t>(rel32) };

        if (std::memcmp(reinterpret_cast<const void*>(target), k_PauseCheckPrologue.data(), k_PauseCheckPrologue.size()) != 0)
        {
            m_Log.Message("[TickGateDetour] ERROR: The pause check does not start with the expected bytes.{}", k_Consequence);
            return;
        }

        s_LoopReturn = loopReturn;
        void* functionAddress{ reinterpret_cast<void*>(target) };
        m_FunctionAddress.store(functionAddress);

        if (!Engine::Base::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedPauseCheck),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[TickGateDetour]", m_Log))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    auto TickGateDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Engine::Base::Hook::System::DisableDetour(m_FunctionAddress.load());
        // note: Uninstalled from RaiseUnhook on the game thread, outside the tick loop, so no call is in flight on that thread.
        if (!Engine::Base::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout))
        {
            m_Log.Message("[TickGateDetour] WARNING: A call was still running when the hook was removed.");
        }
        Engine::Base::Hook::System::RemoveDetour(m_FunctionAddress.load(), "[TickGateDetour]", m_Log);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}