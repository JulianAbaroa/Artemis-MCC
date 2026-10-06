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
}

namespace Platform::Input::Hook
{
    // This function polls keyboard events at a high frequency
    // (approximately every 3ms).
    //
    // Note: This specific handler does not process mouse events.
    //
    // Critical: This function is extremely sensitive to stack manipulation
    // and timing. Avoid using blocking mechanisms (e.g., std::mutex) as they
    // generate undefined behavior.
    //
    // Observed behavior: Blocking or desynchronizing this thread causes
    // the engine to repeatedly execute the received action every 3ms,
    // leading to input flooding.
    //
    // Input Data: Receives a unique 'buttonID', which represents the engine's
    // internal mapping for each physical key.
    auto __fastcall GetButtonStateDetour::HookedGetButtonState(short buttonID) -> char
    {
        if (s_Instance->m_InputStore.IsActionHeld(buttonID)) return 1;

        return s_OriginalFunction(buttonID);
    }

    auto GetButtonStateDetour::Install() -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        void* functionAddress = (void*)m_AOBService.FindPattern(Signature::GetButtonState);
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

        Platform::Hook::System::UninstallDetour(m_FunctionAddress.load(),
            "[GetButtonStateDetour]", m_LogsService);

        m_IsHookInstalled.store(false);
        s_Instance = nullptr;
    }
}