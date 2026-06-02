#include "pch.h"

#include "Thread_Main.h"

#include "Core/Types/EngineStatus.h"

#include "Core/Hooks/Lifecycle/Hook_EngineInitialize.h"
#include "Core/Hooks/Lifecycle/Hook_DestroySubsystems.h"
#include "Core/Hooks/Gametype/Hook_GameEngineInit.h"
#include "Core/Hooks/Render/Hook_Present.h"
#include "Core/Hooks/Render/Hook_ResizeBuffers.h"

#include "Core/States/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Logs/System_Logs.h"

#include <chrono>

using namespace std::chrono_literals;

void Thread_Main::Run() 
{
    this->WaitOrExit(5000ms);

    m_Deps.System_Logs.Log("[MainThread] INFO: Started.");

    bool installed = this->InstallLifecycleHooks();
    if (!installed) this->Shutdown();

    m_Deps.Hook_Present.Install();
    m_Deps.Hook_ResizeBuffers.Install();

    while (m_Deps.State_Lifecycle.IsRunning())
    {
        this->CheckHooksHealth();

		auto engineStatus = m_Deps.State_Lifecycle.GetEngineStatus();
        if (engineStatus == EngineStatus::Destroyed)
        {
            m_Deps.System_Logs.Log("[MainThread] INFO:"
                " Game engine destruction detected, resetting lifecycle.");

            if (!this->IsStillRunning()) break;

            m_Deps.Hook_EngineInitialize.Uninstall();
            m_Deps.Hook_DestroySubsystems.Uninstall();
            m_Deps.Hook_GameEngineInit.Uninstall();

            if (!this->IsStillRunning()) break;

            if (!this->InstallLifecycleHooks()) 
            {
                m_Deps.System_Logs.Log("[MainThread] ERROR:"
                    " Failed to re-install hooks after engine reset.");
                this->Shutdown();
                return;
            }

            m_Deps.State_Lifecycle.SetEngineStatus(
                { EngineStatus::Waiting });
        }

        this->WaitOrExit(1000ms);
    }

    m_Deps.Hook_ResizeBuffers.Uninstall();
    m_Deps.Hook_Present.Uninstall();

    m_Deps.Hook_EngineInitialize.Uninstall();
    m_Deps.Hook_DestroySubsystems.Uninstall();
    m_Deps.Hook_GameEngineInit.Uninstall();

    std::this_thread::sleep_for(200ms);

    m_Deps.System_Logs.Log("[MainThread] INFO: Stopped.");

    HMODULE hMod = m_Deps.State_Lifecycle.GetHandleModule();
    if (hMod != nullptr) FreeLibraryAndExitThread(hMod, 0);
}

bool Thread_Main::WaitOrExit(std::chrono::milliseconds ms)
{
    auto& mutex = m_Deps.State_Lifecycle.GetMutex();
    std::unique_lock<std::mutex> lock(mutex);

    auto& cv = m_Deps.State_Lifecycle.GetCV();
    bool shouldExit = cv.wait_for(lock, ms, [this] {
        return !m_Deps.State_Lifecycle.IsRunning();
    });

    return !shouldExit;
}

bool Thread_Main::InstallLifecycleHooks()
{
    while (m_Deps.State_Lifecycle.IsRunning())
    {
        if (m_Deps.Hook_EngineInitialize.Install() &&
            m_Deps.Hook_DestroySubsystems.Install() &&
            m_Deps.Hook_GameEngineInit.Install())
        {
            return true;
        }

        this->WaitOrExit(1000ms);
    }

    return false;
}

void Thread_Main::Shutdown()
{
    m_Deps.System_Logs.Log("[MainThread] ERROR:"
        " Initiating emergency shutdown.");

    m_Deps.State_Lifecycle.SetRunning(false);
}

void Thread_Main::CheckHooksHealth()
{
    void* engiInitAddr = m_Deps.Hook_EngineInitialize.GetFunctionAddress();
    void* destSubsAddr = m_Deps.Hook_DestroySubsystems.GetFunctionAddress();
    void* gameEngiInitAddr = m_Deps.Hook_GameEngineInit.GetFunctionAddress();

    bool areHooksIntact =
        !this->IsHookIntact(engiInitAddr) ||
        !this->IsHookIntact(destSubsAddr) ||
        !this->IsHookIntact(gameEngiInitAddr);

    if (areHooksIntact && m_Deps.State_Lifecycle.IsRunning())
    {
        m_Deps.System_Logs.Log("[MainThread] WARNING:"
            " Hooks corrupted, rebooting.");

        m_Deps.State_Lifecycle.SetEngineStatus(
            { EngineStatus::Destroyed });
    }
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

bool Thread_Main::IsStillRunning()
{
    this->WaitOrExit(1000ms);

    if (!m_Deps.State_Lifecycle.IsRunning()) return false;

    return true;
}