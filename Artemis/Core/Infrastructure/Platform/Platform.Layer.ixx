export module Platform.Layer;

import Service.Layer;
import Platform.Lifecycle.State;
import Platform.Lifecycle.System;
import Platform.Lifecycle.Hook;
import Platform.Host.State;
import Platform.Host.System;
import Platform.Host.Hook;
import Platform.Input.State;
import Platform.Input.System;
import Platform.Input.Hook;
import Platform.Simulation.State;
import Platform.Simulation.System;
import Platform.Simulation.Hook;
import Platform.Memory.System;
import Platform.Capture.State;
import Platform.Capture.System;
import Platform.Capture.Hook;
import Platform.Bridge.State;
import Platform.Bridge.System;
import Platform.Render.Type;
import Platform.Render.State;
import Platform.Render.System;
import Platform.Render.Hook;
import std;

export namespace Platform
{
    // Root of the Platform layer. Owns the stores, services and hooks that talk to the game process:
    // lifecycle, host (MCC), input, simulation (bots), bridge (Python), memory and render. The hooks are installed by the Runtime layer.
    // note: Members are built in declaration order, so stores come first, then services, then hooks.
    class Layer
    {
    private:
        using milliseconds = std::chrono::milliseconds;

        using LifecycleStore = Platform::Lifecycle::State::LifecycleStore;
        using MatchConfigStore = Platform::Lifecycle::State::MatchConfigStore;
        using MatchCycleStore = Platform::Lifecycle::State::MatchCycleStore;
        using LifecycleService = Platform::Lifecycle::System::LifecycleService;
        using MatchConfigService = Platform::Lifecycle::System::MatchConfigService;
        using EngineInitializeDetour = Platform::Lifecycle::Hook::EngineInitializeDetour;
        using DestroySubsystemsDetour = Platform::Lifecycle::Hook::DestroySubsystemsDetour;
        using EngineInterfaceDetour = Platform::Lifecycle::Hook::EngineInterfaceDetour;
        using LifecycleUpdateDetour = Platform::Lifecycle::Hook::LifecycleUpdateDetour;
        using OpenMapDetour = Platform::Lifecycle::Hook::OpenMapDetour;

        using LocalUserStore = Platform::Host::State::LocalUserStore;
        using HostService = Platform::Host::System::HostService;
        using LocalUserGetPlayerDetour = Platform::Host::Hook::LocalUserGetPlayerDetour;
        using LocalUserGetProfileDetour = Platform::Host::Hook::LocalUserGetProfileDetour;
        using LocalUserGetGamepadMappingDetour = Platform::Host::Hook::LocalUserGetGamepadMappingDetour;
        using CanAcceptAnyJoinRequestDetour = Platform::Host::Hook::CanAcceptAnyJoinRequestDetour;
        using SessionJoinCheckPatch = Platform::Host::Hook::SessionJoinCheckPatch;

        using InputStore = Platform::Input::State::InputStore;
        using MouseDeltaStore = Platform::Input::State::MouseDeltaStore;
        using VirtualInputStore = Platform::Input::State::VirtualInputStore;
        using PlayerActionStore = Platform::Input::State::PlayerActionStore;
        using InputService = Platform::Input::System::InputService;
        using GetButtonStateDetour = Platform::Input::Hook::GetButtonStateDetour;
        using GetRawInputDataDetour = Platform::Input::Hook::GetRawInputDataDetour;
        using WndProcDetour = Platform::Input::Hook::WndProcDetour;
        using InputUpdateDetour = Platform::Input::Hook::InputUpdateDetour;
        using PlayerActionDetour = Platform::Input::Hook::PlayerActionDetour;

        using BotStore = Platform::Simulation::State::BotStore;
        using BotService = Platform::Simulation::System::BotService;
        using SetMachinesDetour = Platform::Simulation::Hook::SetMachinesDetour;

        using BridgeStore = Platform::Bridge::State::BridgeStore;
        using BridgeService = Platform::Bridge::System::BridgeService;

        using AOBService = Platform::Memory::System::AOBService;
        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using CaptureStore = Platform::Capture::State::CaptureStore;
        using CaptureService = Platform::Capture::System::CaptureService;
        using ObjectTableLocator = Platform::Capture::Hook::ObjectTableLocator;
        using PlayerTableLocator = Platform::Capture::Hook::PlayerTableLocator;
        using InteractionTableLocator = Platform::Capture::Hook::InteractionTableLocator;
        using ExecuteGameFrameDetour = Platform::Capture::Hook::ExecuteGameFrameDetour;
        using SimulationTicksDetour = Platform::Capture::Hook::SimulationTicksDetour;

        using FrameContext = Platform::Render::Type::FrameContext;
        using RenderStore = Platform::Render::State::RenderStore;
        using RenderService = Platform::Render::System::RenderService;
        using SwapChainLocator = Platform::Render::System::SwapChainLocator;
        using PresentDetour = Platform::Render::Hook::PresentDetour;
        using ResizeBuffersDetour = Platform::Render::Hook::ResizeBuffersDetour;

    public:
        explicit Layer(Service::Layer& service) :
            m_LifecycleService(service.m_Log, m_LifecycleStore),
            m_MatchConfigService(service.m_Log, service.m_SettingsStore, m_MatchConfigStore),
            m_InputService(service.m_Log, m_InputStore, m_MouseDeltaStore),
            m_AOBService(service.m_Log),
            m_HostService(service.m_Log, m_AOBService),
            m_BotService(service.m_Log, m_AOBService, m_BotStore),
            m_BridgeService(service.m_Log, service.m_SettingsStore, m_LifecycleStore, m_MatchCycleStore, m_PlayerActionStore,
                m_BridgeStore),
            m_CaptureService(service.m_Log, m_MemoryReaderService, m_CaptureStore),
            m_RenderService(service.m_Log, m_RenderStore),
            m_SwapChainLocator(service.m_Log),
            m_EngineInitializeDetour(service.m_Log, m_AOBService, m_LifecycleStore, m_LifecycleService),
            m_DestroySubsystemsDetour(service.m_Log, m_AOBService, m_LifecycleStore, m_LifecycleService),
            m_EngineInterfaceDetour(service.m_Log, m_AOBService, m_MemoryReaderService, m_MatchCycleStore, m_MatchConfigService),
            m_LifecycleUpdateDetour(service.m_Log, m_AOBService, m_MatchConfigStore, m_MatchCycleStore),
            m_OpenMapDetour(service.m_Log, m_AOBService, m_LifecycleService),
            m_LocalUserGetPlayerDetour(service.m_Log, m_HostService, m_LocalUserStore),
            m_LocalUserGetProfileDetour(service.m_Log, m_HostService, m_LocalUserStore),
            m_LocalUserGetGamepadMappingDetour(service.m_Log, m_HostService, m_LocalUserStore),
            m_CanAcceptAnyJoinRequestDetour(service.m_Log, m_AOBService, m_LocalUserStore),
            m_SessionJoinCheckPatch(service.m_Log, m_AOBService, m_LocalUserStore),
            m_GetButtonStateDetour(service.m_Log, m_AOBService, m_InputStore),
            m_GetRawInputDataDetour(service.m_Log, service.m_SettingsStore, m_InputService, m_MouseDeltaStore),
            m_WndProcDetour(service.m_Log, m_InputService, m_LifecycleStore, m_LifecycleService),
            m_InputUpdateDetour(service.m_Log, m_HostService, m_VirtualInputStore),
            m_PlayerActionDetour(service.m_Log, m_AOBService, m_PlayerActionStore, m_BotService, m_BridgeService),
            m_SetMachinesDetour(service.m_Log, m_AOBService, m_BotStore),
            m_PresentDetour(service.m_Log, service.m_TelemetryStore, m_RenderStore, m_RenderService, m_SwapChainLocator),
            m_ResizeBuffersDetour(service.m_Log, m_RenderStore, m_RenderService, m_SwapChainLocator),
            m_ObjectTableLocator(service.m_Log, m_AOBService, m_CaptureStore),
            m_PlayerTableLocator(service.m_Log, m_AOBService, m_CaptureStore),
            m_InteractionTableLocator(service.m_Log, m_AOBService, m_CaptureStore),
            m_ExecuteGameFrameDetour(service.m_Log, service.m_TelemetryStore, m_AOBService, m_LifecycleStore),
            m_SimulationTicksDetour(service.m_Log, service.m_TelemetryStore, m_AOBService, m_LifecycleStore, m_CaptureStore,
                m_ExecuteGameFrameDetour)
        {
            // The button hook needs the engine up, so it is installed on engine initialization.
            // The host hooks also go in here, before the network session forms, so the Artemis users can join it.
            // The bot service finds its functions before the SetMachines hook overwrites the bytes of their signature.
            m_LifecycleService.OnEngineInitialized([this] {
                m_GetButtonStateDetour.Install();
                m_BridgeService.BeginMatch();
                m_PlayerActionDetour.Install();
                m_BotService.ResolveFunctions();
                m_SetMachinesDetour.Install();
                this->InstallHostHooks();
                m_OpenMapDetour.Install();
                m_ObjectTableLocator.FindAndStoreTableBase();
                m_PlayerTableLocator.FindAndStoreTableBase();
                m_InteractionTableLocator.FindAndStoreTableBase();

                // ExecuteGameFrame goes first, so the first SimulationTicks call already counts every tick.
                m_ExecuteGameFrameDetour.Install();
                m_SimulationTicksDetour.Install();
            });

            // The bridge stops first, so a Lockstep wait of the game thread ends before the PlayerAction hook drains.
            m_LifecycleService.OnUnhook([this] {
                m_BridgeService.Stop();
                this->UninstallHostHooks();
                m_SetMachinesDetour.Uninstall();
                m_PlayerActionDetour.Uninstall();
                m_GetButtonStateDetour.Uninstall();
                m_OpenMapDetour.Uninstall();
                m_SimulationTicksDetour.Uninstall();
                m_ExecuteGameFrameDetour.Uninstall();
            });

            // The virtual input is not cleared: the sources picked in the UI stay across matches.
            m_LifecycleService.OnCleanup([this] {
                m_InputStore.Cleanup();
                m_MouseDeltaStore.Cleanup();
                m_LocalUserStore.Cleanup();
                m_PlayerActionStore.Cleanup();
                m_BotStore.Cleanup();
                m_CaptureService.Cleanup();
            });

            // The game window is only known once the swap chain is bound.
            m_RenderService.OnInitialized([this](const FrameContext& frame) {
                m_WndProcDetour.Install(frame.Window);
                m_InputService.BindWindow(frame.Window);
            });

            m_LifecycleService.OnShutdown([this] {
                this->ShutdownRenderAndInput();
            });
        }
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        LifecycleStore m_LifecycleStore{};
        MatchConfigStore m_MatchConfigStore{};
        MatchCycleStore m_MatchCycleStore{};
        LocalUserStore m_LocalUserStore{};
        InputStore m_InputStore{};
        MouseDeltaStore m_MouseDeltaStore{};
        VirtualInputStore m_VirtualInputStore{};
        PlayerActionStore m_PlayerActionStore{};
        BotStore m_BotStore{};
        BridgeStore m_BridgeStore{};
        RenderStore m_RenderStore{};
        CaptureStore m_CaptureStore{};

        // --- System ---
        LifecycleService m_LifecycleService;
        MatchConfigService m_MatchConfigService;
        InputService m_InputService;
        AOBService m_AOBService;
        HostService m_HostService;
        BotService m_BotService;
        BridgeService m_BridgeService;
        MemoryReaderService m_MemoryReaderService{};
        CaptureService m_CaptureService;
        RenderService m_RenderService;
        SwapChainLocator m_SwapChainLocator;

        // --- Hook ---
        EngineInitializeDetour m_EngineInitializeDetour;
        DestroySubsystemsDetour m_DestroySubsystemsDetour;
        EngineInterfaceDetour m_EngineInterfaceDetour;
        LifecycleUpdateDetour m_LifecycleUpdateDetour;
        OpenMapDetour m_OpenMapDetour;
        LocalUserGetPlayerDetour m_LocalUserGetPlayerDetour;
        LocalUserGetProfileDetour m_LocalUserGetProfileDetour;
        LocalUserGetGamepadMappingDetour m_LocalUserGetGamepadMappingDetour;
        CanAcceptAnyJoinRequestDetour m_CanAcceptAnyJoinRequestDetour;
        SessionJoinCheckPatch m_SessionJoinCheckPatch;
        GetButtonStateDetour m_GetButtonStateDetour;
        GetRawInputDataDetour m_GetRawInputDataDetour;
        WndProcDetour m_WndProcDetour;
        InputUpdateDetour m_InputUpdateDetour;
        PlayerActionDetour m_PlayerActionDetour;
        SetMachinesDetour m_SetMachinesDetour;
        PresentDetour m_PresentDetour;
        ResizeBuffersDetour m_ResizeBuffersDetour;
        ObjectTableLocator m_ObjectTableLocator;
        PlayerTableLocator m_PlayerTableLocator;
        InteractionTableLocator m_InteractionTableLocator;
        ExecuteGameFrameDetour m_ExecuteGameFrameDetour;
        SimulationTicksDetour m_SimulationTicksDetour;

    private:
        static constexpr milliseconds k_RenderShutdownTimeout{ 500 };

        // Removes the input hooks, then asks the render to shut down on its next Present and waits for it.
        // The render hooks are removed after that, and the shutdown is forced if it did not finish in time.
        auto ShutdownRenderAndInput() -> void
        {
            m_WndProcDetour.Uninstall();
            m_GetRawInputDataDetour.Uninstall();

            m_RenderService.RequestShutdown();
            m_RenderService.WaitForShutdown(k_RenderShutdownTimeout);

            m_PresentDetour.Uninstall();
            m_ResizeBuffersDetour.Uninstall();

            m_RenderService.Shutdown();
        }

        // Installs the hooks on the game manager of MCC (local users, input) and on the session checks of the engine.
        // note: A failed hook is logged and the rest still go in.
        auto InstallHostHooks() -> void
        {
            m_LocalUserGetPlayerDetour.Install();
            m_LocalUserGetProfileDetour.Install();
            m_LocalUserGetGamepadMappingDetour.Install();
            m_CanAcceptAnyJoinRequestDetour.Install();
            m_SessionJoinCheckPatch.Install();
            m_InputUpdateDetour.Install();
        }

        // Removes the host hooks in reverse order.
        auto UninstallHostHooks() -> void
        {
            m_InputUpdateDetour.Uninstall();
            m_SessionJoinCheckPatch.Uninstall();
            m_CanAcceptAnyJoinRequestDetour.Uninstall();
            m_LocalUserGetGamepadMappingDetour.Uninstall();
            m_LocalUserGetProfileDetour.Uninstall();
            m_LocalUserGetPlayerDetour.Uninstall();
        }
    };
}