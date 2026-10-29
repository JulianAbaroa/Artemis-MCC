module;

#include <windows.h>

module Mod.Thread.System;
import :Main;

import Engine.Base.Hook.Type;
import Engine.Base.Lifecycle.Type;
import Engine.Base.Lifecycle.Hook;
import Engine.Render.Window.Hook;
import Engine.Render.Hook;
import std;

using namespace std::chrono_literals;

namespace
{
    using Engine::Base::Lifecycle::Type::Status;
    using Engine::Base::Lifecycle::Hook::DestroySubsystemsDetour;

    using Engine::Base::Hook::Type::k_JumpRel32Opcode;
}

namespace Mod::Thread::System
{
    auto MainThread::Run() -> void
    {
        auto& lifecycle{ m_Base.m_LifecycleStore };
        auto& logs{ m_Service.m_Log };

        this->WaitOrExit(5000ms);

        logs.Message("[MainThread] INFO: Started.");

        // MCC creates the engine right after loading the game module, before the lifecycle hooks can go in. This hook lives in MCC.
        m_Match.m_EngineInterfaceDetour.Install();

        // The lobby launch (D124) also hooks MCC, which stays loaded for the whole process.
        m_Match.m_LifecycleUpdateDetour.Install();

        if (!this->InstallLifecycleHooks()) this->Shutdown();

        if (lifecycle.IsRunning()) this->InstallRenderHooks();

        while (lifecycle.IsRunning())
        {
            this->CheckHooksHealth();
            m_Service.m_TelemetryService.Update();

            if (lifecycle.GetStatus() == Status::Destroyed)
            {
                logs.Message("[MainThread] INFO: Game engine destruction detected.");

                while (DestroySubsystemsDetour::IsInProgress())
                {
                    std::this_thread::sleep_for(5ms);
                }

                if (!this->IsStillRunning()) break;

                m_Base.m_EngineInitializeDetour.Uninstall();
                m_Base.m_DestroySubsystemsDetour.Uninstall();

                if (!this->IsStillRunning()) break;

                if (!this->InstallLifecycleHooks())
                {
                    logs.Message("[MainThread] ERROR:"
                        " Failed to re-install hooks after engine reset.");

                    this->Shutdown();
                    return;
                }

                lifecycle.SetStatus(Status::Waiting);
            }

            this->WaitOrExit(1000ms);
        }

        m_Base.m_LifecycleService.RaiseShutdown();

        m_Base.m_EngineInitializeDetour.Uninstall();
        m_Base.m_DestroySubsystemsDetour.Uninstall();
        m_Match.m_LifecycleUpdateDetour.Uninstall();
        m_Match.m_EngineInterfaceDetour.Uninstall();

        logs.Message("[MainThread] INFO: Stopped.");
    }

    auto MainThread::WaitOrExit(milliseconds ms) -> bool
    {
        auto& lifecycle{ m_Base.m_LifecycleStore };

        std::unique_lock<std::mutex> lock{ lifecycle.GetShutdownMutex() };

        bool shouldExit{ lifecycle.GetShutdownCV().wait_for(lock, ms, [&] {
            return !lifecycle.IsRunning();
        }) };

        return !shouldExit;
    }

    auto MainThread::InstallLifecycleHooks() -> bool
    {
        while (m_Base.m_LifecycleStore.IsRunning())
        {
            if (m_Base.m_EngineInitializeDetour.Install() &&
                m_Base.m_DestroySubsystemsDetour.Install())
            {
                return true;
            }

            this->WaitOrExit(1000ms);
        }

        return false;
    }

    auto MainThread::InstallRenderHooks() -> bool
    {
        constexpr int k_MaxAttempts{ 5 };

        for (int attempt = 0; attempt < k_MaxAttempts &&
            m_Base.m_LifecycleStore.IsRunning(); ++attempt)
        {
            if (m_Render.m_PresentDetour.Install() &&
                m_Render.m_ResizeBuffersDetour.Install())
            {
                m_Render.m_GetRawInputDataDetour.Install();
                return true;
            }

            this->WaitOrExit(1000ms);
        }

        m_Service.m_Log.Message("[MainThread] ERROR:"
            " Failed to install the render hooks, Artemis keeps running without them.");

        return false;
    }

    auto MainThread::Shutdown() -> void
    {
        m_Service.m_Log.Message("[MainThread] ERROR:"
            " Initiating emergency shutdown.");

        m_Base.m_LifecycleService.SignalShutdown();
    }

    auto MainThread::CheckHooksHealth() -> void
    {
        auto& lifecycle{ m_Base.m_LifecycleStore };

        if (lifecycle.GetStatus() == Status::Destroyed) return;
        if (DestroySubsystemsDetour::IsInProgress()) return;

        void* engineInitAddr{ m_Base.m_EngineInitializeDetour.GetFunctionAddress() };
        void* destroyAddr{ m_Base.m_DestroySubsystemsDetour.GetFunctionAddress() };

        bool areHooksCorrupted{
            !this->IsHookIntact(engineInitAddr) ||
            !this->IsHookIntact(destroyAddr) };

        if (areHooksCorrupted && lifecycle.IsRunning())
        {
            m_Service.m_Log.Message("[MainThread] WARNING:"
                " Hooks corrupted, rebooting.");

            lifecycle.SetStatus(Status::Destroyed);
        }
    }

    auto MainThread::IsHookIntact(void* address) -> bool
    {
        if (address == nullptr) return false;

        unsigned char firstByte{};
        SIZE_T bytesRead{};

        if (ReadProcessMemory(GetCurrentProcess(), address,
            &firstByte, 1, &bytesRead))
        {
            return firstByte == k_JumpRel32Opcode;
        }

        return false;
    }

    auto MainThread::IsStillRunning() -> bool
    {
        this->WaitOrExit(1000ms);

        return m_Base.m_LifecycleStore.IsRunning();
    }
}