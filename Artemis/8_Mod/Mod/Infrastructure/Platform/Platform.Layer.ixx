export module Platform.Layer;

import Service.Layer;
import Engine.Base.Layer;
import Engine.Capture.Layer;
import Engine.Control.Layer;
import Engine.Match.Layer;
import Engine.Render.Layer;
import Bridge.Transport.Layer;
import Platform.Port.System;
import Platform.Render.Type;
import std;

export namespace Platform
{
    // Composes the Layers of the engine projects and of Bridge.Transport with the port adapters between them (D215), and connects the
    // lifecycle callbacks and the installation of the hooks, which the Layers do not do (D242). The hooks are installed by the Runtime layer.
    // note: Temporary until Mod.Layer (M8 C.3). Members are built in declaration order: the Layers of the engine, then the adapters.
    class Layer
    {
    private:
        using milliseconds = std::chrono::milliseconds;

        using FrameContext = Platform::Render::Type::FrameContext;

        using BridgeEngineAdapter = Platform::Port::System::BridgeEngineAdapter;
        using BridgeActionSource = Platform::Port::System::BridgeActionSource;

    public:
        explicit Layer(Service::Layer& service) :
            m_Base(service),
            m_Capture(service, m_Base),
            m_Control(service, m_Base),
            m_Match(service, m_Base),
            m_Render(service, m_Base),
            m_BridgeEngineAdapter(m_Base.m_LifecycleStore, m_Match.m_MatchCycleStore, m_Control.m_PlayerActionStore),
            m_Transport(service, m_BridgeEngineAdapter),
            m_BridgeActionSource(m_Transport.m_BridgeService)
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
        }
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // The Layers of the engine in the order they are built, then the port adapters and the transport.
        Engine::Base::Layer m_Base;
        Engine::Capture::Layer m_Capture;
        Engine::Control::Layer m_Control;
        Engine::Match::Layer m_Match;
        Engine::Render::Layer m_Render;
        BridgeEngineAdapter m_BridgeEngineAdapter;
        ::Bridge::Transport::Layer m_Transport;
        BridgeActionSource m_BridgeActionSource;

    private:
        static constexpr milliseconds k_RenderShutdownTimeout{ 500 };

        // Removes the input hooks, then asks the render to shut down on its next Present and waits for it.
        // The render hooks are removed after that, and the shutdown is forced if it did not finish in time.
        auto ShutdownRenderAndInput() -> void
        {
            m_Render.m_WndProcDetour.Uninstall();
            m_Render.m_GetRawInputDataDetour.Uninstall();

            m_Render.m_RenderService.RequestShutdown();
            m_Render.m_RenderService.WaitForShutdown(k_RenderShutdownTimeout);

            m_Render.m_PresentDetour.Uninstall();
            m_Render.m_ResizeBuffersDetour.Uninstall();

            m_Render.m_RenderService.Shutdown();
        }

        // Installs the hooks on the game manager of MCC (local users, input) and on the session checks of the engine.
        // note: A failed hook is logged and the rest still go in.
        auto InstallHostHooks() -> void
        {
            m_Base.m_LocalUserGetPlayerDetour.Install();
            m_Base.m_LocalUserGetProfileDetour.Install();
            m_Base.m_LocalUserGetGamepadMappingDetour.Install();
            m_Base.m_CanAcceptAnyJoinRequestDetour.Install();
            m_Base.m_SessionJoinCheckPatch.Install();
            m_Control.m_InputUpdateDetour.Install();
        }

        // Removes the host hooks in reverse order.
        auto UninstallHostHooks() -> void
        {
            m_Control.m_InputUpdateDetour.Uninstall();
            m_Base.m_SessionJoinCheckPatch.Uninstall();
            m_Base.m_CanAcceptAnyJoinRequestDetour.Uninstall();
            m_Base.m_LocalUserGetGamepadMappingDetour.Uninstall();
            m_Base.m_LocalUserGetProfileDetour.Uninstall();
            m_Base.m_LocalUserGetPlayerDetour.Uninstall();
        }
    };
}