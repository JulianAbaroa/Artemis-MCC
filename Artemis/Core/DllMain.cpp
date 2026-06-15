#include "pch.h"

#include "DllMain.h"
#include "Core.h"

#include "Proxy/ProxyExports.h"

#include "States/Core_State.h"
#include "States/Other/Lifecycle/State_Lifecycle.h"
#include "States/Other/Settings/State_Settings.h"

#include "Systems/Core_System.h"
#include "Systems/Other/Settings/System_Settings.h"
#include "Systems/Other/Logs/System_Logs.h"

#include "Threads/Core_Thread.h"
#include "Threads/Main/Thread_Main.h"
#include "Threads/AI/Thread_AI.h"
#include "Threads/Input/Thread_Input.h"

#include "External/minhook/include/MinHook.h"

#include <fstream>
#include <thread>

#pragma comment(lib, "shlwapi.lib")

// TODO: Refactor UI/Utils/
// TODO: Make a Color Library for the UI.

DllInstance g_DllInstance;

static std::unique_ptr<Core> m_Mod;

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

BOOL DllInstance::OnAttach(HMODULE hModule)
{
    DisableThreadLibraryCalls(hModule);

    HANDLE hThread = CreateThread(NULL, 0, DllInstance::InitializeArtemis, hModule, 0, NULL);
    if (hThread) CloseHandle(hThread);

    return TRUE;
}

void DllInstance::OnDetach(LPVOID lpReserved)
{
    this->DeinitializeArtemis(lpReserved);
}

// Responsible for initializing Artemis data and core systems.
DWORD WINAPI DllInstance::InitializeArtemis(LPVOID lpParam)
{
    // Initializes the complete mod architecture.
    m_Mod = std::make_unique<Core>();
    m_Mod->Initialize();

    // Saves the handle module of the game.
    HMODULE handleModule = (HMODULE)lpParam;
    m_Mod->State->Lifecycle->SetHandleModule(handleModule);

    // Gets the path of the game.
    char buffer[MAX_PATH];
    GetModuleFileNameA(handleModule, buffer, MAX_PATH);
    PathRemoveFileSpecA(buffer);

    // Initializes the paths that are going to be used by the mod.
    m_Mod->System->Settings->InitializePaths(buffer);
    std::ofstream ofs(m_Mod->State->Settings->
        GetLoggerPath(), std::ios::trunc);

    // Load user preferences if user allowed the mod to use AppData.
    if (m_Mod->State->Settings->ShouldUseAppData())
    {
        // m_Mod->System->Preferences->LoadPreferences();
    }

    // Initializes MinHook, the tool that Artemis uses to hook game's functions.
    if (MH_Initialize() != MH_OK)
    {
        m_Mod->System->Logs->Log("[DllMain] ERROR:"
            " MH_Initialize failed.");
        return 0;
    }

    // Sets the mod IsRunning flag as true.
    m_Mod->State->Lifecycle->SetRunning(true);

    // Creates and starts the core threads of Artemis.

    g_DllInstance.m_MainThread = std::thread(
        &Thread_Main::Run, m_Mod->Thread->Main.get());

    g_DllInstance.m_InputThread = std::thread(
        &Thread_Input::Run, m_Mod->Thread->Input.get());

    g_DllInstance.m_AIThread = std::thread(
        &Thread_AI::Run, m_Mod->Thread->AI.get());

    m_Mod->System->Logs->Log("[DllMain] INFO:"
        " Artemis Initialized.");
    return 0;
}

void DllInstance::DeinitializeArtemis(LPVOID lpReserved)
{
    m_Mod->System->Logs->Log("[DllMain] DETACH lpReserved=%p", lpReserved);

    m_Mod->State->Lifecycle->SetRunning(false);

    if (lpReserved != NULL)
    {
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (m_MainThread.joinable())  m_MainThread.detach();
    if (m_InputThread.joinable()) m_InputThread.detach();
    if (m_AIThread.joinable())    m_AIThread.detach();
    m_Mod->Deinitialize();
    MH_Uninitialize();
}