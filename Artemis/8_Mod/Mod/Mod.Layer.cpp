module;

#include <windows.h>
#include "External/minhook/include/MinHook.h"

module Mod.Layer;

import Engine.Render.Type;
import std;

namespace
{
    using Engine::Render::Type::FrameContext;
}

using namespace std::chrono_literals;

namespace Mod
{
    Layer::Layer(HMODULE handleModule) : m_Module(handleModule)
    {
        // The lifecycle wiring of the pure components lives here, not in them (D178). The one of the engine is WireEngine, a member.
        // Every layer is cleaned up in the order of its dependencies, the ones that read first: Environment (which includes Relations),
        // Egocentric and Export, then Tables, Map and Resolved. The tables read the resolved data, which is built from the map (D195, D200).
        auto& lifecycle = m_Base.m_LifecycleService;

        lifecycle.OnMapFileOpened([this](const std::string& filePath) {
            m_Map.m_MapLoaderService.OpenFile(filePath);
        });

        lifecycle.OnCleanup([this] {
            m_Environment.Cleanup();
            m_Egocentric.Cleanup();
            m_Export.Cleanup();
            m_Tables.Cleanup();
            m_Map.Cleanup();
            m_Resolved.Cleanup();
        });

        m_AI.AddObserver([this](const ObservedTick& tick) {
            this->OnTick(tick);
        });
    }

    auto Layer::Start() -> bool
    {
        auto& logs = m_Service.m_Log;

        char buffer[MAX_PATH]{};
        GetModuleFileNameA(m_Module, buffer, MAX_PATH);
        std::string directory = std::filesystem::path(buffer).parent_path().string();

        m_Service.m_SettingsService.InitializePaths(directory);
        std::ofstream{ m_Service.m_SettingsStore.GetLoggerPath(), std::ios::trunc };

        if (m_Service.m_SettingsStore.IsAppDataEnabled())
        {
            m_Preferences.Load();
        }

        if (MH_Initialize() != MH_OK)
        {
            logs.Message("[Core] ERROR: MH_Initialize failed.");
            return false;
        }

        m_IsMinHookReady = true;

        m_Base.m_LifecycleStore.SetRunning(true);

        logs.Message("[Core] INFO: Artemis initialized.");
        return true;
    }

    auto Layer::Run() -> void
    {
        m_AIThread = std::jthread([this] { m_AI.Run(); });
        m_BenchThread = std::jthread([this] { m_Bench.Run(); });
        m_BridgeThread = std::jthread([this] { m_Bridge.Run(); });
        m_DumpThread = std::jthread([this] { m_Dump.Run(); });

        m_Main.Run();
    }

    auto Layer::RequestShutdown() -> void
    {
        m_Base.m_LifecycleService.SignalShutdown();
    }

    Layer::~Layer()
    {
        m_Preferences.Save();

        m_Base.m_LifecycleService.SignalShutdown();

        m_Base.m_LifecycleService.RaiseUnhook();

        m_Base.m_LifecycleService.RaiseShutdown();

        // Gives the other threads time to react to the shutdown signal and the raises.
        std::this_thread::sleep_for(200ms);

        if (m_IsMinHookReady) MH_Uninitialize();
    }

    auto Layer::WireEngine() -> bool
    {
        // The action source is connected before any hook goes in (D243).
        m_Control.m_PlayerActionDetour.SetActionSource(m_BridgeActionSource);

        // The host hooks also go in here, before the network session forms, so the Artemis users can join it.
        // The bot service finds its functions before the SetMachines hook overwrites the bytes of their signature.
        m_Base.m_LifecycleService.OnEngineInitialized([this] {
            m_Transport.m_BridgeService.BeginMatch();
            m_Control.m_PlayerActionDetour.Install();
            m_Control.m_BotService.ResolveFunctions();
            m_Control.m_SetMachinesDetour.Install();
            this->InstallHostHooks();
            m_Base.m_OpenMapDetour.Install();
            m_Capture.m_ObjectTableLocator.FindAndStoreTableBase();
            m_Capture.m_PlayerTableLocator.FindAndStoreTableBase();
            m_Capture.m_InteractionTableLocator.FindAndStoreTableBase();

            // ExecuteGameFrame goes first, so the first SimulationTicks call already counts every tick.
            m_Capture.m_ExecuteGameFrameDetour.Install();
            m_Capture.m_SimulationTicksDetour.Install();
        });

        // The bridge stops first, so a Lockstep wait of the game thread ends before the PlayerAction hook drains.
        m_Base.m_LifecycleService.OnUnhook([this] {
            m_Transport.m_BridgeService.Stop();
            this->UninstallHostHooks();
            m_Control.m_SetMachinesDetour.Uninstall();
            m_Control.m_PlayerActionDetour.Uninstall();
            m_Base.m_OpenMapDetour.Uninstall();
            m_Capture.m_SimulationTicksDetour.Uninstall();
            m_Capture.m_ExecuteGameFrameDetour.Uninstall();
        });

        // The virtual input is not cleared: the sources picked in the UI stay across matches.
        m_Base.m_LifecycleService.OnCleanup([this] {
            m_Base.m_LocalUserStore.Cleanup();
            m_Control.m_PlayerActionStore.Cleanup();
            m_Control.m_BotStore.Cleanup();
            m_Capture.m_CaptureService.Cleanup();
        });

        // The game window is only known once the swap chain is bound.
        m_Render.m_RenderService.OnInitialized([this](const FrameContext& frame) {
            m_Render.m_WndProcDetour.Install(frame.Window);
        });

        m_Base.m_LifecycleService.OnShutdown([this] {
            this->ShutdownRenderAndInput();
        });

        return true;
    }

    auto Layer::OnTick(const ObservedTick& tick) -> void
    {
        const bool isViewerActive{ m_Viewer.m_CameraStore.IsActive() };

        m_Viewer.m_CollidableSceneService.Update(isViewerActive);

        m_UI.m_TickTraceUIService.CaptureClock(*tick.Snapshot);
        m_UI.m_TickTraceUIService.Record(tick.Generation);
        if (m_UI.m_TickDumpUIService.IsRecording())
        {
            m_UI.m_TickDumpUIService.Push(tick.Tick, tick.Snapshot, tick.Generation, tick.DroppedBefore,
                tick.IsFirstAfterLoad, isViewerActive);
        }
    }

    auto Layer::ShutdownRenderAndInput() -> void
    {
        m_Render.m_WndProcDetour.Uninstall();
        m_Render.m_GetRawInputDataDetour.Uninstall();

        m_Render.m_RenderService.RequestShutdown();
        m_Render.m_RenderService.WaitForShutdown(k_RenderShutdownTimeout);

        m_Render.m_PresentDetour.Uninstall();
        m_Render.m_ResizeBuffersDetour.Uninstall();

        m_Render.m_RenderService.Shutdown();
    }

    auto Layer::InstallHostHooks() -> void
    {
        m_Base.m_LocalUserGetPlayerDetour.Install();
        m_Base.m_LocalUserGetProfileDetour.Install();
        m_Base.m_LocalUserGetGamepadMappingDetour.Install();
        m_Base.m_CanAcceptAnyJoinRequestDetour.Install();
        m_Base.m_SessionJoinCheckPatch.Install();
        m_Control.m_InputUpdateDetour.Install();
    }

    auto Layer::UninstallHostHooks() -> void
    {
        m_Control.m_InputUpdateDetour.Uninstall();
        m_Base.m_SessionJoinCheckPatch.Uninstall();
        m_Base.m_CanAcceptAnyJoinRequestDetour.Uninstall();
        m_Base.m_LocalUserGetGamepadMappingDetour.Uninstall();
        m_Base.m_LocalUserGetProfileDetour.Uninstall();
        m_Base.m_LocalUserGetPlayerDetour.Uninstall();
    }
}