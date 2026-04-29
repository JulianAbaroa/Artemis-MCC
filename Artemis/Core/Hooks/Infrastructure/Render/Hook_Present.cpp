#include "pch.h"
#include "Core/UI/Core_UI.h"
#include "Core/UI/UI_Main.h"
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"
#include "Core/Hooks/Infrastructure/Render/Hook_Present.h"
#include "Core/Hooks/Infrastructure/Input/Hook_GetRawInputData.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Render.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Render.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"

// Intercepts the DXGI Present call to handle frame updates, UI rendering, 
// and frame capture logic before the buffer is displayed on screen.
HRESULT __stdcall Hook_Present::HookedPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (g_pState->Infrastructure->Render->IsResizing())
    {
        return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    if (!g_pSystem->Infrastructure->Render->IsInitialized())
    {
        g_pSystem->Infrastructure->Render->Initialize(pSwapChain);
    }

    // Rebuild ImGui UI
    if (g_pState->Infrastructure->Render->ShouldRebuildFonts())
    {
        g_pSystem->Infrastructure->Render->UpdateUIScale();
    }

    // Calculate Framerate 
    g_pSystem->Infrastructure->Render->UpdateFramerate();

    // Draw ImGui
    if (g_pState->Infrastructure->Render->GetRTV())
    {
        g_pSystem->Infrastructure->Render->BeginFrame(pSwapChain);
        g_pUI->Main->Draw();
        g_pSystem->Infrastructure->Render->EndFrame();
    }

    return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
}

void Hook_Present::Install() 
{
    if (m_PresentHookInstalled.load()) return;

    auto addresses = g_pSystem->Infrastructure->Render->GetVtableAddresses();
    if (!addresses.Present) 
    {
        g_pSystem->Debug->Log("[Present] ERROR: Failed to obtain the function address.");
        return;
    }

    m_PresentAddress = addresses.Present;
    if (MH_CreateHook(m_PresentAddress, &this->HookedPresent, reinterpret_cast<LPVOID*>(&m_OriginalPresent)) != MH_OK) 
    {
        g_pSystem->Debug->Log("[Present] ERROR: Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_PresentAddress) != MH_OK) 
    {
        g_pSystem->Debug->Log("[Present] ERROR: Failed to enable the hook.");
        return;
    }

	g_pHook->Infrastructure->GetRawInputData->Install();

    m_PresentHookInstalled.store(true);
    g_pSystem->Debug->Log("[Present] INFO: Hook installed.");
}

void Hook_Present::Uninstall() 
{
    if (!m_PresentHookInstalled.load()) return;

    g_pSystem->Infrastructure->Render->Shutdown();

    if (m_PresentAddress)
    {
        MH_DisableHook(m_PresentAddress);
        MH_RemoveHook(m_PresentAddress);
    }

    g_pHook->Infrastructure->GetRawInputData->Uninstall();

    m_PresentHookInstalled.store(false);
    g_pSystem->Debug->Log("[Present] INFO: Hook uninstalled.");
}