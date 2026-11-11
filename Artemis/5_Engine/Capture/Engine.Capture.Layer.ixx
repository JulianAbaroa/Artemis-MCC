export module Engine.Capture.Layer;

import Services.Layer;
import Engine.Base.Layer;
import Engine.Capture.State;
import Engine.Capture.System;
import Engine.Capture.Oracle.State;
import Engine.Capture.Oracle.System;
import Engine.Capture.Hook;

export namespace Engine::Capture
{
    // The capture of the engine: the locators of the object, player and interaction tables, and the frame and simulation tick hooks.
    // note: Owner only: it has no callbacks and installs nothing. The one that composes it connects the lifecycle and installs the hooks.
    class Layer
    {
    private:
        using CaptureStore = Engine::Capture::State::CaptureStore;
        using CaptureService = Engine::Capture::System::CaptureService;
        using OracleStore = Engine::Capture::Oracle::State::OracleStore;
        using OracleService = Engine::Capture::Oracle::System::OracleService;
        using ObjectTableLocator = Engine::Capture::Hook::ObjectTableLocator;
        using PlayerTableLocator = Engine::Capture::Hook::PlayerTableLocator;
        using InteractionTableLocator = Engine::Capture::Hook::InteractionTableLocator;
        using ExecuteGameFrameDetour = Engine::Capture::Hook::ExecuteGameFrameDetour;
        using SimulationTicksDetour = Engine::Capture::Hook::SimulationTicksDetour;

    public:
        Layer(Services::Layer& service, Engine::Base::Layer& base) :
            m_CaptureService(service.m_Log, base.m_LifecycleStore, base.m_MemoryReaderService, m_CaptureStore, service.m_TelemetryStore),
            m_OracleService(service.m_Log, base.m_AOBService, base.m_LifecycleStore, m_CaptureStore, m_OracleStore),
            m_ObjectTableLocator(service.m_Log, base.m_AOBService, m_CaptureStore),
            m_PlayerTableLocator(service.m_Log, base.m_AOBService, m_CaptureStore),
            m_InteractionTableLocator(service.m_Log, base.m_AOBService, m_CaptureStore),
            m_ExecuteGameFrameDetour(service.m_Log, service.m_TelemetryStore, base.m_AOBService, base.m_LifecycleStore, m_OracleService),
            m_SimulationTicksDetour(service.m_Log, service.m_TelemetryStore, base.m_AOBService, base.m_LifecycleStore, m_CaptureStore,
                base.m_PresetService, m_ExecuteGameFrameDetour) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        CaptureStore m_CaptureStore{};
        OracleStore m_OracleStore{};

        // --- System ---
        CaptureService m_CaptureService;
        OracleService m_OracleService;

        // --- Hook ---
        ObjectTableLocator m_ObjectTableLocator;
        PlayerTableLocator m_PlayerTableLocator;
        InteractionTableLocator m_InteractionTableLocator;
        ExecuteGameFrameDetour m_ExecuteGameFrameDetour;
        SimulationTicksDetour m_SimulationTicksDetour;
    };
}