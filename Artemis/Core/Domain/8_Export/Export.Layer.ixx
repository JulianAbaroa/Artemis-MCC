export module Export.Layer;

import Service.Layer;
import Platform.Layer;
import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Tick.State;
import Export.Tick.System;
import Export.Tick.Hook;
import Export.Flat.State;
import Export.Flat.System;

export namespace Export
{
    class Layer
    {
    private:
        using TickStore = Export::Tick::State::TickStore;
        using TickService = Export::Tick::System::TickService;
        using ExecuteGameFrameDetour = Export::Tick::Hook::ExecuteGameFrameDetour;
        using SimulationTicksDetour = Export::Tick::Hook::SimulationTicksDetour;
        using FlatStore = Export::Flat::State::FlatStore;
        using FlatService = Export::Flat::System::FlatService;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform,
            Resolved::Layer& resolved, Tables::Layer& tables, Relations::Layer& relations,
            Environment::Layer& environment, Egocentric::Layer& egocentric) :
            m_TickService(tables.m_ObjectStore, tables.m_PlayerStore, tables.m_InteractionStore, relations.m_ClassifierStore, relations.m_ObjectGraphStore, environment.m_EntitiesStore, environment.m_PlayersStore, environment.m_VitalityDetailsStore, environment.m_ZoneDetailsStore, environment.m_TeleporterDetailsStore, environment.m_LiftDetailsStore, environment.m_BarrierDetailsStore, environment.m_AimStore, environment.m_EventsStore, environment.m_MatchStore, egocentric.m_SelfStore, egocentric.m_AffordanceStore, egocentric.m_RaycastStore, resolved.m_DefinitionsStore, m_TickStore),
            m_ExecuteGameFrameDetour(service.m_Log, service.m_TelemetryStore, platform.m_AOBService,
                platform.m_LifecycleStore),
            m_SimulationTicksDetour(service.m_Log, service.m_TelemetryStore, platform.m_AOBService, platform.m_LifecycleStore,
                platform.m_CaptureStore, m_ExecuteGameFrameDetour)
        {
            auto& lifecycle = platform.m_LifecycleService;

            // ExecuteGameFrame goes first, so the first SimulationTicks call already counts every tick.
            lifecycle.OnEngineInitialized([this] {
                m_ExecuteGameFrameDetour.Install();
                m_SimulationTicksDetour.Install();
            });

            lifecycle.OnUnhook([this] {
                m_SimulationTicksDetour.Uninstall();
                m_ExecuteGameFrameDetour.Uninstall();
            });

            lifecycle.OnCleanup([this] {
                m_TickStore.Cleanup();
                m_FlatStore.Cleanup();
            });
        }
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        TickStore m_TickStore{};
        FlatStore m_FlatStore{};

        // --- System ---
        TickService m_TickService;
        FlatService m_FlatService{};

        // --- Hook ---
        ExecuteGameFrameDetour m_ExecuteGameFrameDetour;
        SimulationTicksDetour m_SimulationTicksDetour;
    };
}