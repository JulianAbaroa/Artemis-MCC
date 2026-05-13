#include "pch.h"

// Header.
#include "DllMain.h"

// Proxies.
#include "Proxy/ProxyExports.h"

// Mod Core.
#include "Mod_Core.h"

// --- States ---
#include "States/Core_State.h"
#include "States/Infrastructure/Core_State_Infrastructure.h"

#include "States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"
#include "States/Infrastructure/Persistence/State_Settings.h"

// --- Systems ---
#include "Systems/Core_System.h"
#include "Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Systems/Infrastructure/Persistence/System_Settings.h"
#include "Systems/Infrastructure/Persistence/System_Preferences.h"

#include "Systems/Interface/System_Debug.h"

// --- Threads ---
#include "Threads/Core_Thread.h"

#include "Threads/Domain/Thread_Main.h"
#include "Threads/Domain/Thread_AI.h"

#include "Threads/Infrastructure/Thread_Input.h"

// MinHook.
#include "External/minhook/include/MinHook.h"

#include <fstream>
#pragma comment(lib, "shlwapi.lib")

using namespace std::chrono_literals;

ModLoader g_DllInstance;

extern "C" BOOL APIENTRY DllMain(HMODULE handleModule, 
    DWORD ulReasonForCall, LPVOID lpReserved) 
{
    switch (ulReasonForCall) 
    {
        case DLL_PROCESS_ATTACH: 
            return g_DllInstance.OnAttach(handleModule);
            
        case DLL_PROCESS_DETACH: 
            g_DllInstance.OnDetach(lpReserved);
            break;
    }

    return TRUE;
}

BOOL ModLoader::OnAttach(HMODULE hModule)
{
    DisableThreadLibraryCalls(hModule);

    HANDLE hThread = CreateThread(NULL, 0, ModLoader::InitializeArtemis, hModule, 0, NULL);
    if (hThread) CloseHandle(hThread);

    return TRUE;
}

void ModLoader::OnDetach(LPVOID lpReserved)
{
    this->DeinitializeArtemis(lpReserved);
}


// Responsible for initializing Artemis data and core systems.
DWORD WINAPI ModLoader::InitializeArtemis(LPVOID lpParam)
{
    // Initializes the complete mod architecture.
    g_Mod = std::make_unique<Mod_Core>();

    // Saves the handle module of the game.
    HMODULE handleModule = (HMODULE)lpParam;
    g_pState->Infrastructure->Lifecycle->SetHandleModule(handleModule);

    // Gets the path of the game.
    char buffer[MAX_PATH];
    GetModuleFileNameA(handleModule, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer);

    // Initializes the paths that are going to be used by the mod.
    g_pSystem->Infrastructure->Settings->InitializePaths(buffer);
    std::ofstream ofs(g_pState->Infrastructure->Settings->GetLoggerPath(), std::ios::trunc);
    
    // Load user preferences if user allowed the mod to use AppData.
    if (g_pState->Infrastructure->Settings->ShouldUseAppData())
    {
        g_pSystem->Infrastructure->Preferences->LoadPreferences();
    }

    // Initializes MinHook, the tool that Artemis uses to hook game's functions.
    if (MH_Initialize() != MH_OK)
    {
        g_pSystem->Debug->Log("[DllMain] ERROR: MH_Initialize failed.");
        return 0;
    }

    // Sets the mod IsRunning flag as true.
    g_pState->Infrastructure->Lifecycle->SetRunning(true);

    // Creates and starts the core threads of Artemis.
    g_DllInstance.m_MainThread = std::thread(&Thread_Main::Run, g_pThread->Main.get());
    g_DllInstance.m_InputThread = std::thread(&Thread_Input::Run, g_pThread->Input.get());
    g_DllInstance.m_AIThread = std::thread(&Thread_AI::Run, g_pThread->AI.get());

    g_pSystem->Debug->Log("[DllMain] INFO: Artemis Initialized.");
    return 0;
}

void ModLoader::DeinitializeArtemis(LPVOID lpReserved)
{
    if (lpReserved == NULL)
    {
        g_pSystem->Debug->Log("[DllMain] INFO: Deinitializing Artemis.");

        g_pState->Infrastructure->Lifecycle->SetRunning(false);

        std::this_thread::sleep_for(100ms);

        MH_Uninitialize();

        if (m_MainThread.joinable()) m_MainThread.detach();
        if (m_InputThread.joinable()) m_InputThread.detach();
        if (m_AIThread.joinable()) m_AIThread.detach();

        g_Mod.reset();

        g_pSystem->Debug->Log("[DllMain] INFO: Artemis Deinitialized.");
    }
}