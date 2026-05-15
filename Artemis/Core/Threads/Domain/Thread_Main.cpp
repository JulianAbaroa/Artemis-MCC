#include "pch.h"

// Header.
#include "Thread_Main.h"

// Types.
#include "Core/Types/Infrastructure/EngineStatus.h"

// --- Hooks ---

#include "Core/Hooks/Infrastructure/Lifecycle/Hook_EngineInitialize.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_DestroySubsystems.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_GameEngineInit.h"
#include "Core/Hooks/Infrastructure/Render/Hook_Present.h"
#include "Core/Hooks/Infrastructure/Render/Hook_ResizeBuffers.h"

// --- States ---

#include "Core/States/Infrastructure/Engine/Lifecycle/State_Lifecycle.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Thread/System_Thread.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_Main::Run() 
{
    // Initial delay.
    m_Deps.System_Thread.WaitOrExit(5000ms);

    m_Deps.System_Debug.Log("[MainThread] INFO: Started.");

    this->InitializeArtemis();
    this->InstallUIHooks();

    while (m_Deps.State_Lifecycle.IsRunning())
    {
        this->CheckHooksHealth();

		auto engineStatus = m_Deps.State_Lifecycle.GetEngineStatus();
        if (engineStatus == EngineStatus::Destroyed)
        {
            m_Deps.System_Debug.Log("[MainThread] INFO:"
                " Game engine destruction detected, resetting lifecycle.");

            if (!this->IsStillRunning()) break;

            this->DeinitializeArtemis();

            if (!this->IsStillRunning()) break;

            if (!this->TryInstallLifecycleHooks("Engine Reset Cycle")) 
            {
                m_Deps.System_Debug.Log("[MainThread] ERROR:"
                    " Failed to re-install hooks after engine reset.");
                Shutdown();
                return;
            }

            m_Deps.State_Lifecycle.SetEngineStatus(
                { EngineStatus::Waiting });
        }

        m_Deps.System_Thread.WaitOrExit(1000ms);
    }

    this->UninstallUIHooks();
    this->DeinitializeArtemis();

    std::this_thread::sleep_for(200ms);
    m_Deps.System_Debug.Log("[MainThread] INFO: Stopped.");

    HMODULE hMod = m_Deps.State_Lifecycle.GetHandleModule();
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
    m_Deps.Hook_EngineInitialize.Uninstall();
    m_Deps.Hook_DestroySubsystems.Uninstall();
    m_Deps.Hook_GameEngineInit.Uninstall();
}

void Thread_Main::InstallUIHooks()
{
    m_Deps.Hook_Present.Install();
    m_Deps.Hook_ResizeBuffers.Install();
}

void Thread_Main::UninstallUIHooks()
{
    m_Deps.Hook_ResizeBuffers.Uninstall();
    m_Deps.Hook_Present.Uninstall();
}

void Thread_Main::CheckHooksHealth()
{
    bool areHooksIntact =
        !this->IsHookIntact(m_Deps.Hook_EngineInitialize.GetFunctionAddress()) ||
        !this->IsHookIntact(m_Deps.Hook_DestroySubsystems.GetFunctionAddress()) ||
        !this->IsHookIntact(m_Deps.Hook_GameEngineInit.GetFunctionAddress());

    if (areHooksIntact && m_Deps.State_Lifecycle.IsRunning())
    {
        m_Deps.System_Debug.Log("[MainThread] WARNING:"
            " Hooks corrupted, rebooting.");

        m_Deps.State_Lifecycle.SetEngineStatus(
            { EngineStatus::Destroyed });
    }
}

bool Thread_Main::IsStillRunning()
{
    m_Deps.System_Thread.WaitOrExit(1000ms);

    if (!m_Deps.State_Lifecycle.IsRunning()) return false;

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
    while (m_Deps.State_Lifecycle.IsRunning())
    {
        if (m_Deps.Hook_EngineInitialize.Install() &&
            m_Deps.Hook_DestroySubsystems.Install() &&
            m_Deps.Hook_GameEngineInit.Install())
        {
            return true;
        }

        m_Deps.System_Thread.WaitOrExit(1000ms);
    }

    return false;
}

void Thread_Main::Shutdown()
{
    m_Deps.System_Debug.Log("[MainThread] ERROR:"
        " Initiating emergency shutdown.");

    m_Deps.State_Lifecycle.SetRunning(false);
}