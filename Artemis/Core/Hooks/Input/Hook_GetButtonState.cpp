#include "pch.h"

#include "Hook_GetButtonState.h"

#include "Core/Types/AOB/Signatures.h"

#include "Core/States/Input/State_Input.h"

#include "Core/Systems/Memory/AOBScanner/System_AOBScanner.h"
#include "Core/Systems/Logs/System_Logs.h"

#include "External/minhook/include/MinHook.h"

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
char __fastcall Hook_GetButtonState::HookedGetButtonState(short buttonID)
{
    auto nextInput = s_Instance->m_Deps.State_Input.GetNextRequest();
    if (nextInput.Action != InputAction::Unknown)
    {
        if (static_cast<short>(nextInput.Action) == buttonID)
        {
            return 1;
        }
    }

    return m_OriginalFunction(buttonID);
}

Hook_GetButtonState* Hook_GetButtonState::s_Instance = nullptr;

void Hook_GetButtonState::Install()
{
    if (m_IsHookInstalled.load()) return;
    s_Instance = this;

    void* functionAddress = (void*)s_Instance->m_Deps.
        System_AOBScanner.FindPattern(Signatures::GetButtonState);

    if (!functionAddress)
    {
        s_Instance->m_Deps.System_Logs.Log("[GetButtonState] ERROR:"
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
        s_Instance->m_Deps.System_Logs.Log("[GetButtonState] ERROR:"
            " Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
    {
        s_Instance->m_Deps.System_Logs.Log("[GetButtonState] ERROR:"
            " Failed to enable the hook.");
        return;
    }

    m_IsHookInstalled.store(true);
    s_Instance->m_Deps.System_Logs.Log("[GetButtonState] INFO:"
        " Hook installed.");
}

void Hook_GetButtonState::Uninstall()
{
    if (!m_IsHookInstalled.load()) return;

    MH_DisableHook(m_FunctionAddress.load());
    MH_RemoveHook(m_FunctionAddress.load());

    m_IsHookInstalled.store(false);

    s_Instance->m_Deps.System_Logs.Log("[GetButtonState] INFO:"
        " Hook uninstalled.");

    s_Instance = nullptr;
}