#include "pch.h"

#include "Hook_Present.h"

#include "Core/States/Render/State_Render.h"

#include "Core/Systems/Render/System_Render.h"
#include "Core/Systems/Logs/System_Logs.h"

#include "Core/Hooks/Input/Hook_GetRawInputData.h"
#include "Core/Hooks/Window/Hook_WndProc.h"

#include "Core/UI/Core_UI.h"
#include "Core/UI/Launcher/UI_Launcher.h"
#include "Core/UI/Overlay/UI_Overlay.h"
#include "Core/UI/ObjectTable/UI_ObjectTable.h"
#include "Core/UI/PlayerTable/UI_PlayerTable.h"
#include "Core/UI/Map/UI_Map.h"
#include "Core/UI/MemoryScanner/UI_MemoryScanner.h"
#include "Core/UI/Settings/UI_Settings.h"
#include "Core/UI/Logs/UI_Logs.h"

#include "External/minhook/include/MinHook.h"

// Intercepts the DXGI Present call to handle frame updates, UI rendering, 
// and frame capture logic before the buffer is displayed on screen.
HRESULT __stdcall Hook_Present::HookedPresent(
    IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (s_Instance->m_Deps.State_Render.IsResizing())
    {
        return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
    }

    if (!s_Instance->m_Deps.System_Render.IsInitialized())
    {
        s_Instance->m_Deps.System_Render.Initialize(pSwapChain);

        DXGI_SWAP_CHAIN_DESC sd;
        pSwapChain->GetDesc(&sd);
        s_Instance->m_Deps.Hook_WndProc.Install(sd.OutputWindow);
    }

    // Rebuild ImGui UI
    if (s_Instance->m_Deps.State_Render.ShouldRebuildFonts())
    {
        s_Instance->m_Deps.System_Render.UpdateUIScale();
    }

    // Calculate Framerate 
    s_Instance->m_Deps.System_Render.UpdateFramerate();

    // Draw ImGui
    if (s_Instance->m_Deps.State_Render.GetRTV())
    {
        s_Instance->m_Deps.System_Render.BeginFrame(pSwapChain);

        s_Instance->m_Deps.UI_Launcher.DrawDockSpace();
        s_Instance->m_Deps.UI_Launcher.Draw();
        s_Instance->m_Deps.UI_Overlay.Draw();

        s_Instance->m_Deps.UI_ObjectTable.Draw();
        s_Instance->m_Deps.UI_PlayerTable.Draw();
        s_Instance->m_Deps.UI_Map.Draw();
        s_Instance->m_Deps.UI_Settings.Draw();
        s_Instance->m_Deps.UI_MemoryScanner.Draw();
        s_Instance->m_Deps.UI_Logs.Draw();

        s_Instance->m_Deps.System_Render.EndFrame();
    }

    return m_OriginalPresent(pSwapChain, SyncInterval, Flags);
}

Hook_Present* Hook_Present::s_Instance = nullptr;

void Hook_Present::Install() 
{
    if (m_PresentHookInstalled.load()) return;
    s_Instance = this;

    auto addresses = s_Instance->m_Deps.System_Render.GetVtableAddresses();
    if (!addresses.Present) 
    {
        s_Instance->m_Deps.System_Logs.Log("[Present] ERROR:"
            " Failed to obtain the function address.");
        return;
    }

    m_PresentAddress = addresses.Present;
    if (MH_CreateHook(
            m_PresentAddress, 
            &this->HookedPresent, 
            reinterpret_cast<LPVOID*>(&m_OriginalPresent)) 
        != MH_OK) 
    {
        s_Instance->m_Deps.System_Logs.Log("[Present] ERROR:"
            " Failed to create the hook.");
        return;
    }
    if (MH_EnableHook(m_PresentAddress) != MH_OK) 
    {
        s_Instance->m_Deps.System_Logs.Log("[Present] ERROR:"
            " Failed to enable the hook.");
        return;
    }

    s_Instance->m_Deps.Hook_GetRawInputData.Install();

    m_PresentHookInstalled.store(true);
    s_Instance->m_Deps.System_Logs.Log("[Present] INFO: Hook installed.");
}

void Hook_Present::Uninstall()
{
    if (!m_PresentHookInstalled.load()) return;

    s_Instance->m_Deps.Hook_WndProc.Uninstall();
    s_Instance->m_Deps.System_Render.Shutdown();

    if (m_PresentAddress)
    {
        MH_DisableHook(m_PresentAddress);
        MH_RemoveHook(m_PresentAddress);
    }

    s_Instance->m_Deps.Hook_GetRawInputData.Uninstall();

    m_PresentHookInstalled.store(false);
    s_Instance->m_Deps.System_Logs.Log("[Present] INFO: Hook uninstalled.");
    s_Instance = nullptr;
}