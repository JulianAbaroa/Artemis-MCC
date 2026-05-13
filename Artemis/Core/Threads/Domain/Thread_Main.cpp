#include "pch.h"

// Header.
#include "Thread_Main.h"

// --- Hooks ---
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"

#include "Core/Hooks/Infrastructure/Lifecycle/Hook_EngineInitialize.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_DestroySubsystems.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_GameEngineInit.h"
#include "Core/Hooks/Infrastructure/Render/Hook_Present.h"
#include "Core/Hooks/Infrastructure/Render/Hook_ResizeBuffers.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Infrastructure/Engine/Thread/System_Thread.h"

#include "Core/Systems/Interface/System_Debug.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_Main::Run() 
{
    auto& lifecycle = *g_pState->Infrastructure->Lifecycle;
    auto& thread = *g_pSystem->Infrastructure->Thread;
    auto& debug = *g_pSystem->Debug;

    // Initial delay.
    thread.WaitOrExit(5000ms);

    debug.Log("[MainThread] INFO: Started.");

    this->InitializeArtemis();
    this->InstallUIHooks();

    while (lifecycle.IsRunning())
    {
        this->CheckHooksHealth();

		auto engineStatus = lifecycle.GetEngineStatus();
        if (engineStatus == EngineStatus::Destroyed)
        {
            debug.Log("[MainThread] INFO: Game engine destruction detected, resetting lifecycle.");

            if (!this->IsStillRunning()) break;

            this->DeinitializeArtemis();

            if (!this->IsStillRunning()) break;

            if (!this->TryInstallLifecycleHooks("Engine Reset Cycle")) 
            {
                debug.Log("[MainThread] ERROR: Failed to re-install hooks after engine reset.");
                Shutdown();
                return;
            }

            lifecycle.SetEngineStatus({ EngineStatus::Waiting });
        }

        thread.WaitOrExit(1000ms);
    }

    this->UninstallUIHooks();
    this->DeinitializeArtemis();

    std::this_thread::sleep_for(200ms);
    debug.Log("[MainThread] INFO: Stopped.");

    HMODULE hMod = lifecycle.GetHandleModule();
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
    auto& enginInitialize = *g_pHook->Infrastructure->EngineInitialize;
    enginInitialize.Uninstall();

    auto& destroySubsystems = *g_pHook->Infrastructure->DestroySubsystems;
    destroySubsystems.Uninstall();

    auto& gameEngineStart = *g_pHook->Infrastructure->GameEngineStart;
    gameEngineStart.Uninstall();
}

void Thread_Main::InstallUIHooks()
{
    auto& present = *g_pHook->Infrastructure->Present;
    present.Install();

    auto& resizeBuffers = *g_pHook->Infrastructure->ResizeBuffers;
    resizeBuffers.Install();
}

void Thread_Main::UninstallUIHooks()
{
    auto& resizeBuffers = *g_pHook->Infrastructure->ResizeBuffers;
    resizeBuffers.Uninstall();

    auto& present = *g_pHook->Infrastructure->Present;
    present.Uninstall();
}

void Thread_Main::CheckHooksHealth()
{
    auto& enginInitialize = *g_pHook->Infrastructure->EngineInitialize;
    auto& destroySubsystems = *g_pHook->Infrastructure->DestroySubsystems;
    auto& gameEngineStart = *g_pHook->Infrastructure->GameEngineStart;

    bool areHooksIntact =
        !this->IsHookIntact(enginInitialize.GetFunctionAddress()) ||
        !this->IsHookIntact(destroySubsystems.GetFunctionAddress()) ||
        !this->IsHookIntact(gameEngineStart.GetFunctionAddress());

    auto& lifecycle = *g_pState->Infrastructure->Lifecycle;
    if (areHooksIntact && lifecycle.IsRunning())
    {
        auto& debug = *g_pSystem->Debug;
        debug.Log("[MainThread] WARNING: Hooks corrupted, rebooting.");

        auto& lifecycle = *g_pState->Infrastructure->Lifecycle;
        lifecycle.SetEngineStatus({ EngineStatus::Destroyed });
    }
}

bool Thread_Main::IsStillRunning()
{
    auto& thread = *g_pSystem->Infrastructure->Thread;
    thread.WaitOrExit(1000ms);

    auto& lifecycle = *g_pState->Infrastructure->Lifecycle;
    if (!lifecycle.IsRunning()) return false;

    return true;
}


bool Thread_Main::IsHookIntact(void* address)
{
    if (address == nullptr) return false;

    unsigned char firstByte{};
    size_t bytesRead;

    if (ReadProcessMemory(GetCurrentProcess(), 
        address, &firstByte, 1, &bytesRead))
    {
        return firstByte == 0xE9;
    }

    return false;
}

bool Thread_Main::TryInstallLifecycleHooks(const char* context)
{
    auto& enginInitialize = *g_pHook->Infrastructure->EngineInitialize;
    auto& destroySubsystems = *g_pHook->Infrastructure->DestroySubsystems;
    auto& gameEngineStart = *g_pHook->Infrastructure->GameEngineStart;
    auto& lifecycle = *g_pState->Infrastructure->Lifecycle;
    auto& thread = *g_pSystem->Infrastructure->Thread;

    while (lifecycle.IsRunning())
    {
        if (enginInitialize.Install() &&
            destroySubsystems.Install() &&
            gameEngineStart.Install())
        {
            return true;
        }

        thread.WaitOrExit(1000ms);
    }

    return false;
}

void Thread_Main::Shutdown()
{
    auto& debug = *g_pSystem->Debug;
    debug.Log("[MainThread] ERROR: Initiating emergency shutdown.");

    auto& lifecycle = *g_pState->Infrastructure->Lifecycle;
    lifecycle.SetRunning(false);
}