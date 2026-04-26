#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Data/CoreDataHook.h"
#include "Core/Hooks/Lifecycle/CoreLifecycleHook.h"
#include "Core/Hooks/Lifecycle/EngineInitializeHook.h"
#include "Core/Hooks/Lifecycle/DestroySubsystemsHook.h"
#include "Core/Hooks/Lifecycle/GameEngineStartHook.h"
#include "Core/Hooks/Render/CoreRenderHook.h"
#include "Core/Hooks/Render/PresentHook.h"
#include "Core/Hooks/Render/ResizeBuffersHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/InputState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ThreadSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "Core/Threads/Domain/MainThread.h"
#include <chrono>

#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Map/MapSystem.h"

using namespace std::chrono_literals;

void MainThread::Run() 
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

void MainThread::InitializeArtemis()
{
    if (!this->TryInstallLifecycleHooks("Initial Boot"))
    {
        this->Shutdown();
        return;
    }
}

void MainThread::DeinitializeArtemis()
{
    g_pHook->Lifecycle->EngineInitialize->Uninstall();
    g_pHook->Lifecycle->DestroySubsystems->Uninstall();
    g_pHook->Lifecycle->GameEngineStart->Uninstall();
}

void MainThread::InstallCaptureHooks()
{
    // Video
    g_pHook->Render->Present->Install();
    g_pHook->Render->ResizeBuffers->Install();
}

void MainThread::UninstallCaptureHooks()
{
    // Video
    g_pHook->Render->ResizeBuffers->Uninstall();
    g_pHook->Render->Present->Uninstall();
}

void MainThread::CheckHooksHealth()
{
    bool areHooksIntact =
        !this->IsHookIntact(g_pHook->Lifecycle->EngineInitialize->GetFunctionAddress()) ||
        !this->IsHookIntact(g_pHook->Lifecycle->DestroySubsystems->GetFunctionAddress()) ||
        !this->IsHookIntact(g_pHook->Lifecycle->GameEngineStart->GetFunctionAddress());

    if (areHooksIntact && g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        g_pSystem->Debug->Log("[MainThread] WARNING: Hooks corrupted, rebooting.");
        g_pState->Infrastructure->Lifecycle->SetEngineStatus({ EngineStatus::Destroyed });
    }
}

bool MainThread::IsStillRunning()
{
    g_pSystem->Infrastructure->Thread->WaitOrExit(1000ms);
    if (!g_pState->Infrastructure->Lifecycle->IsRunning()) return false;
    return true;
}


bool MainThread::IsHookIntact(void* address)
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

bool MainThread::TryInstallLifecycleHooks(const char* context)
{
    while (g_pState->Infrastructure->Lifecycle->IsRunning())
    {
        if (g_pHook->Lifecycle->EngineInitialize->Install(true) &&
            g_pHook->Lifecycle->DestroySubsystems->Install(true) &&
            g_pHook->Lifecycle->GameEngineStart->Install(true))
        {
            return true;
        }

        g_pSystem->Infrastructure->Thread->WaitOrExit(1000ms);
    }

    return false;
}

void MainThread::Shutdown()
{
    g_pSystem->Debug->Log("[MainThread] ERROR: Initiating emergency shutdown.");
    g_pState->Infrastructure->Lifecycle->SetRunning(false);
}