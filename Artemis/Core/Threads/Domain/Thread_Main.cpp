#include "pch.h"
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_EngineInitialize.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_DestroySubsystems.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_GameEngineInit.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"
#include "Core/Hooks/Infrastructure/Render/Hook_Present.h"
#include "Core/Hooks/Infrastructure/Render/Hook_ResizeBuffers.h"
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"
#include "Core/States/Infrastructure/Engine/State_Lifecycle.h"
#include "Core/States/Infrastructure/Engine/State_Input.h"
#include "Core/States/Infrastructure/Persistence/State_Settings.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Thread.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "Core/Threads/Domain/Thread_Main.h"
#include <chrono>

#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Map/System_Map.h"

using namespace std::chrono_literals;

void Thread_Main::Run() 
{
    // Initial delay.
    g_pSystem->Infrastructure->Thread->WaitOrExit(5000ms);

    g_pSystem->Debug->Log("[MainThread] INFO: Started.");

    this->InitializeArtemis();
    this->InstallCaptureHooks();

    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        this->CheckHooksHealth();

		auto engineStatus = g_pState->Infrastructure->Lifecycle->GetEngineStatus();
        if (engineStatus == EngineStatus::Destroyed)
        {
            g_pSystem->Debug->Log("[MainThread] INFO: Game engine destruction detected, resetting lifecycle.");

            if (!this->IsStillRunning()) break;

            this->DeinitializeArtemis();

            if (!this->IsStillRunning()) break;

            if (!this->TryInstallLifecycleHooks("Engine Reset Cycle")) 
            {
                g_pSystem->Debug->Log("[MainThread] ERROR: Failed to re-install hooks after engine reset.");
                Shutdown();
                return;
            }

            g_pState->Infrastructure->Lifecycle->SetEngineStatus(
                { EngineStatus::Waiting });
        }

        g_pSystem->Infrastructure->Thread->WaitOrExit(1000ms);
    }

    this->UninstallCaptureHooks();
    this->DeinitializeArtemis();

    std::this_thread::sleep_for(200ms);
    g_pSystem->Debug->Log("[MainThread] INFO: Stopped.");

    HMODULE hMod = g_pState->Infrastructure->Lifecycle->GetHandleModule();
    if (hMod != nullptr) FreeLibraryAndExitThread(hMod, 0);
}

void Thread_Main::InitializeArtemis()
{
    if (!this->TryInstallLifecycleHooks("Initial Boot"))
    {
        this->Shutdown();
        return;
    }
}

void Thread_Main::DeinitializeArtemis()
{
    g_pHook->Infrastructure->EngineInitialize->Uninstall();
    g_pHook->Infrastructure->DestroySubsystems->Uninstall();
    g_pHook->Infrastructure->GameEngineStart->Uninstall();
}

void Thread_Main::InstallCaptureHooks()
{
    // Video
    g_pHook->Infrastructure->Present->Install();
    g_pHook->Infrastructure->ResizeBuffers->Install();
}

void Thread_Main::UninstallCaptureHooks()
{
    // Video
    g_pHook->Infrastructure->ResizeBuffers->Uninstall();
    g_pHook->Infrastructure->Present->Uninstall();
}

void Thread_Main::CheckHooksHealth()
{
    bool areHooksIntact =
        !this->IsHookIntact(g_pHook->Infrastructure->EngineInitialize->GetFunctionAddress()) ||
        !this->IsHookIntact(g_pHook->Infrastructure->DestroySubsystems->GetFunctionAddress()) ||
        !this->IsHookIntact(g_pHook->Infrastructure->GameEngineStart->GetFunctionAddress());

    if (areHooksIntact && g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        g_pSystem->Debug->Log("[MainThread] WARNING: Hooks corrupted, rebooting.");
        g_pState->Infrastructure->Lifecycle->SetEngineStatus({ EngineStatus::Destroyed });
    }
}

bool Thread_Main::IsStillRunning()
{
    g_pSystem->Infrastructure->Thread->WaitOrExit(1000ms);
    if (!g_pState->Infrastructure->Lifecycle->IsRunning()) return false;
    return true;
}


bool Thread_Main::IsHookIntact(void* address)
{
    if (address == nullptr) return false;

    unsigned char firstByte{};
    size_t bytesRead;

    if (ReadProcessMemory(GetCurrentProcess(), address, &firstByte, 1, &bytesRead))
    {
        return firstByte == 0xE9;
    }

    return false;
}

bool Thread_Main::TryInstallLifecycleHooks(const char* context)
{
    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        if (g_pHook->Infrastructure->EngineInitialize->Install(true) &&
            g_pHook->Infrastructure->DestroySubsystems->Install(true) &&
            g_pHook->Infrastructure->GameEngineStart->Install(true))
        {
            return true;
        }

        g_pSystem->Infrastructure->Thread->WaitOrExit(1000ms);
    }

    return false;
}

void Thread_Main::Shutdown()
{
    g_pSystem->Debug->Log("[MainThread] ERROR: Initiating emergency shutdown.");
    g_pState->Infrastructure->Lifecycle->SetRunning(false);
}