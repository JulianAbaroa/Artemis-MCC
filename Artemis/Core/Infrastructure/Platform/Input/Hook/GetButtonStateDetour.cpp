module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Input.Hook;
import :GetButtonState;

import Platform.Memory.Type;
import Platform.Hook.System;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using Platform::Hook::System::k_DrainTimeout;
}

namespace Platform::Input::Hook
{
    // Polls the keyboard at a high frequency (about every 3 ms). Mouse events do not go through it.
    // param buttonID: Engine mapping of a physical key.
    // note: Very sensitive to stack manipulation and timing. Blocking it (a std::mutex, for example) desynchronizes the thread
    // and the engine repeats the received action every 3 ms (input flooding).
    auto __fastcall GetButtonStateDetour::HookedGetButtonState(short buttonID) -> char
    {
        Platform::Hook::System::InFlightScope scope{ s_InFlight };

        if (s_Instance->m_InputStore.IsActionHeld(buttonID)) return 1;

        return s_OriginalFunction(buttonID);
    }

    auto GetButtonStateDetour::Install() -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        void* functionAddress = (void*)m_AOBService.FindPattern(Signature::k_GetButtonState);
        m_FunctionAddress.store(functionAddress);

        if (!Platform::Hook::System::InstallDetour(functionAddress,
            reinterpret_cast<void*>(&HookedGetButtonState),
            reinterpret_cast<void**>(&s_OriginalFunction),
            "[GetButtonStateDetour]", m_LogsService))
        {
            return;
        }

        m_IsHookInstalled.store(true);
    }

    auto GetButtonStateDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        Platform::Hook::System::DisableDetour(m_FunctionAddress.load());
        if (!Platform::Hook::System::WaitForDrain(s_InFlight, k_DrainTimeout))
        {
            m_LogsService.Message("[GetButtonStateDetour] WARNING:"
                " A call was still running when the hook was removed.");
        }
        Platform::Hook::System::RemoveDetour(m_FunctionAddress.load(),
            "[GetButtonStateDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}