#include "pch.h"

#include "Thread_Main.h"

#include "Core/Types/Other/EngineStatus.h"

#include "Core/Hooks/Other/Lifecycle/Hook_EngineInitialize.h"
#include "Core/Hooks/Other/Lifecycle/Hook_DestroySubsystems.h"
#include "Core/Hooks/Other/Render/Hook_Present.h"
#include "Core/Hooks/Other/Render/Hook_ResizeBuffers.h"

#include "Core/States/Other/Lifecycle/State_Lifecycle.h"

#include "Core/Systems/Other/Telemetry/System_Telemetry.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

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
        m_Deps.System_Telemetry.Update();

		auto engineStatus = m_Deps.State_Lifecycle.GetStatus();
        if (engineStatus == Status::Destroyed)
        {
            m_Deps.System_Logs.Log("[MainThread] INFO:"
                " Game engine destruction detected.");

            while (Hook_DestroySubsystems::s_InProgress.load(
                std::memory_order_acquire))
            {
                std::this_thread::sleep_for(5ms);
            }

            if (!this->IsStillRunning()) break;

            m_Deps.Hook_EngineInitialize.Uninstall();
            m_Deps.Hook_DestroySubsystems.Uninstall();

            if (!this->IsStillRunning()) break;

            if (!this->InstallLifecycleHooks()) 
            {
                m_Deps.System_Logs.Log("[MainThread] ERROR:"
                    " Failed to re-install hooks after engine reset.");
                this->Shutdown();
                return;
            }

            m_Deps.State_Lifecycle.SetStatus(
                { Status::Waiting });
        }

        this->WaitOrExit(1000ms);
    }

    m_Deps.Hook_ResizeBuffers.Uninstall();
    m_Deps.Hook_Present.Uninstall();

    m_Deps.Hook_EngineInitialize.Uninstall();
    m_Deps.Hook_DestroySubsystems.Uninstall();

    std::this_thread::sleep_for(200ms);

    m_Deps.System_Logs.Log("[MainThread] INFO: Stopped.");

    HMODULE hMod = m_Deps.State_Lifecycle.GetHandleModule();
    if (hMod != nullptr) FreeLibraryAndExitThread(hMod, 0);
}

bool Thread_Main::WaitOrExit(std::chrono::milliseconds ms)
{
    auto& mutex = m_Deps.State_Lifecycle.GetShutdownMutex();
    std::unique_lock<std::mutex> lock(mutex);

    auto& cv = m_Deps.State_Lifecycle.GetShutdownCV();
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
            m_Deps.Hook_DestroySubsystems.Install())
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
    if (m_Deps.State_Lifecycle.GetStatus() == 
        Status::Destroyed) return;

    if (Hook_DestroySubsystems::s_InProgress.load(
        std::memory_order_acquire)) return;

    void* engiInitAddr = m_Deps.Hook_EngineInitialize.GetFunctionAddress();
    void* destSubsAddr = m_Deps.Hook_DestroySubsystems.GetFunctionAddress();

    bool areHooksIntact =
        !this->IsHookIntact(engiInitAddr) ||
        !this->IsHookIntact(destSubsAddr);

    if (areHooksIntact && m_Deps.State_Lifecycle.IsRunning())
    {
        m_Deps.System_Logs.Log("[MainThread] WARNING:"
            " Hooks corrupted, rebooting.");

        m_Deps.State_Lifecycle.SetStatus(
            { Status::Destroyed });
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