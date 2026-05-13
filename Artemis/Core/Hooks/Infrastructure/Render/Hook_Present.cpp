#include "pch.h"

// Header.
#include "Hook_Present.h"

// --- Hooks ---

#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"

#include "Core/Hooks/Infrastructure/Input/Hook_GetRawInputData.h"

// --- States ---

#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/States/Infrastructure/Engine/Render/State_Render.h"

// --- Systems ---

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Infrastructure/Engine/Render/System_Render.h"

#include "Core/Systems/Interface/System_Debug.h"

// --- UI ---

#include "Core/UI/Core_UI.h"

#include "Core/UI/UI_Main.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

// Intercepts the DXGI Present call to handle frame updates, UI rendering, 
// and frame capture logic before the buffer is displayed on screen.
HRESULT __stdcall Hook_Present::HookedPresent(
    IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    auto& stateRender = *g_pState->Infrastructure->Render;
    auto& systemRender = *g_pSystem->Infrastructure->Render;

    if (stateRender.IsResizing())
    {
        return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    if (!systemRender.IsInitialized()) 
    {
        systemRender.Initialize(pSwapChain);
    }

    // Rebuild ImGui UI
    if (stateRender.ShouldRebuildFonts())
    {
        systemRender.UpdateUIScale();
    }

    // Calculate Framerate 
    systemRender.UpdateFramerate();

    // Draw ImGui
    if (stateRender.GetRTV())
    {
        systemRender.BeginFrame(pSwapChain);
        g_pUI->Main->Draw();
        systemRender.EndFrame();
    }

    return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
}

void Hook_Present::Install() 
{
    if (m_PresentHookInstalled.load()) return;

    auto& debug = *g_pSystem->Debug;
    auto& systemRender = *g_pSystem->Infrastructure->Render;

    auto addresses = systemRender.GetVtableAddresses();
    if (!addresses.Present) 
    {
        debug.Log("[Present] ERROR: Failed to obtain the function address.");
        return;
    }

    m_PresentAddress = addresses.Present;
    if (MH_CreateHook(
            m_PresentAddress, 
            &this->HookedPresent, 
            reinterpret_cast<LPVOID*>(&m_OriginalPresent)) 
        != MH_OK) 
    {
        debug.Log("[Present] ERROR: Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_PresentAddress) != MH_OK) 
    {
        debug.Log("[Present] ERROR: Failed to enable the hook.");
        return;
    }

    auto& getRawInputData = *g_pHook->Infrastructure->GetRawInputData;
    getRawInputData.Install();

    m_PresentHookInstalled.store(true);
    debug.Log("[Present] INFO: Hook installed.");
}

void Hook_Present::Uninstall() 
{
    if (!m_PresentHookInstalled.load()) return;

    auto& systemRender = *g_pSystem->Infrastructure->Render;
    systemRender.Shutdown();

    if (m_PresentAddress)
    {
        MH_DisableHook(m_PresentAddress);
        MH_RemoveHook(m_PresentAddress);
    }

    auto& getRawInputData = *g_pHook->Infrastructure->GetRawInputData;
    getRawInputData.Uninstall();

    m_PresentHookInstalled.store(false);

    auto& debug = *g_pSystem->Debug;
    debug.Log("[Present] INFO: Hook uninstalled.");
}