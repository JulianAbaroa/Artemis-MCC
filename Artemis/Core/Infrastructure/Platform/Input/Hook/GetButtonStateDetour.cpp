module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Platform.Input.Hook;
import :GetButtonState;

import Platform.Input.Type;
import Platform.Memory.Type;

namespace
{
    namespace Signature = Platform::Memory::Type::Signature;

    using InputAction = Platform::Input::Type::Action;
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
    // the engine to repeatedly execute the recieved action every 3ms, 
    // leading to input flooding.
    // 
    // Input Data: Recieves a unique 'buttonID', which represents the engine's 
    // internal mapping for each physical key.
    auto __fastcall GetButtonStateDetour::HookedGetButtonState(short buttonID) -> char
    {
        auto nextInput = s_Instance->m_InputStore.GetNextRequest();
        if (nextInput.Action != InputAction::Unknown)
        {
            if (static_cast<short>(nextInput.Action) == buttonID)
            {
                return 1;
            }
        }

        return m_OriginalFunction(buttonID);
    }

    GetButtonStateDetour* GetButtonStateDetour::s_Instance = nullptr;

    auto GetButtonStateDetour::Install() -> void
    {
        if (m_IsHookInstalled.load()) return;
        s_Instance = this;

        void* functionAddress = (void*)s_Instance->m_AOBService.FindPattern(
            Signature::GetButtonState);

        if (!functionAddress)
        {
            s_Instance->m_LogsService.Message("[GetButtonStateDetour] ERROR:"
                " Failed to obtain the function address.");
            return;
        }

        m_FunctionAddress.store(functionAddress);
        if (MH_CreateHook(
            m_FunctionAddress.load(),
            &this->HookedGetButtonState,
            reinterpret_cast<LPVOID*>(&m_OriginalFunction))
            != MH_OK)
        {
            s_Instance->m_LogsService.Message("[GetButtonStateDetour] ERROR:"
                " Failed to create the hook.");
            return;
        }
        if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
        {
            s_Instance->m_LogsService.Message("[GetButtonStateDetour] ERROR:"
                " Failed to enable the hook.");
            return;
        }

        m_IsHookInstalled.store(true);
        s_Instance->m_LogsService.Message("[GetButtonStateDetour] INFO:"
            " Hook installed.");
    }

    auto GetButtonStateDetour::Uninstall() -> void
    {
        if (!m_IsHookInstalled.load()) return;

        MH_DisableHook(m_FunctionAddress.load());
        MH_RemoveHook(m_FunctionAddress.load());

        m_IsHookInstalled.store(false);

        s_Instance->m_LogsService.Message("[GetButtonStateDetour] INFO:"
            " Hook uninstalled.");

        s_Instance = nullptr;
    }
}