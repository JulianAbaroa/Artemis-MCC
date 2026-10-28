export module Engine.Capture.Layer;

import Service.Layer;
import Engine.Base.Layer;
import Platform.Capture.State;
import Platform.Capture.System;
import Platform.Capture.Hook;

export namespace Engine::Capture
{
    // The capture of the engine: the locators of the object, player and interaction tables, and the frame and simulation tick hooks.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the lifecycle and installs the hooks.
    class Layer
    {
    private:
        using CaptureStore = Platform::Capture::State::CaptureStore;
        using CaptureService = Platform::Capture::System::CaptureService;
        using ObjectTableLocator = Platform::Capture::Hook::ObjectTableLocator;
        using PlayerTableLocator = Platform::Capture::Hook::PlayerTableLocator;
        using InteractionTableLocator = Platform::Capture::Hook::InteractionTableLocator;
        using ExecuteGameFrameDetour = Platform::Capture::Hook::ExecuteGameFrameDetour;
        using SimulationTicksDetour = Platform::Capture::Hook::SimulationTicksDetour;

    public:
        Layer(Service::Layer& service, Engine::Base::Layer& base) :
            m_CaptureService(service.m_Log, base.m_MemoryReaderService, m_CaptureStore),
            m_ObjectTableLocator(service.m_Log, base.m_AOBService, m_CaptureStore),
            m_PlayerTableLocator(service.m_Log, base.m_AOBService, m_CaptureStore),
            m_InteractionTableLocator(service.m_Log, base.m_AOBService, m_CaptureStore),
            m_ExecuteGameFrameDetour(service.m_Log, service.m_TelemetryStore, base.m_AOBService, base.m_LifecycleStore),
            m_SimulationTicksDetour(service.m_Log, service.m_TelemetryStore, base.m_AOBService, base.m_LifecycleStore, m_CaptureStore,
                m_ExecuteGameFrameDetour) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        CaptureStore m_CaptureStore{};

        // --- System ---
        CaptureService m_CaptureService;

        // --- Hook ---
        ObjectTableLocator m_ObjectTableLocator;
        PlayerTableLocator m_PlayerTableLocator;
        InteractionTableLocator m_InteractionTableLocator;
        ExecuteGameFrameDetour m_ExecuteGameFrameDetour;
        SimulationTicksDetour m_SimulationTicksDetour;
    };
}