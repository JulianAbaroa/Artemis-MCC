export module Bridge.Codec.Layer;

import Blam.MapModel.Layer;
import Blam.Runtime.Layer;
import Perception.Layer;
import Views.Layer;
import Bridge.Codec.Tick.State;
import Bridge.Codec.Tick.System;
import Bridge.Codec.Flat.State;
import Bridge.Codec.Flat.System;
import Bridge.Codec.Pipeline.System;

export namespace Bridge::Codec
{
    // Packs what the other layers built into the Tick, and flattens it into the BridgeTick for the client.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using TickStore = Bridge::Codec::Tick::State::TickStore;
        using TickService = Bridge::Codec::Tick::System::TickService;
        using FlatStore = Bridge::Codec::Flat::State::FlatStore;
        using FlatService = Bridge::Codec::Flat::System::FlatService;
        using PipelineService = Bridge::Codec::Pipeline::System::PipelineService;

    public:
        Layer(Blam::MapModel::Layer& resolved, Blam::Runtime::Layer& tables,
            Perception::Layer& environment, Views::Layer& egocentric) :
            m_TickService(tables.m_ObjectStore, tables.m_PlayerStore, tables.m_InteractionStore, environment.m_ClassifierStore, environment.m_ObjectGraphStore, environment.m_EntitiesStore, environment.m_PlayersStore, environment.m_VitalityDetailsStore, environment.m_ZoneDetailsStore, environment.m_TeleporterDetailsStore, environment.m_LiftDetailsStore, environment.m_BarrierDetailsStore, environment.m_AimStore, environment.m_EventsStore, environment.m_MatchStore, egocentric.m_SelfStore, egocentric.m_AffordanceStore, egocentric.m_RaycastStore, resolved.m_DefinitionsStore, m_TickStore),
            m_PipelineService(tables, environment, egocentric, m_TickService) {}
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        TickStore m_TickStore{};
        FlatStore m_FlatStore{};

        // --- System ---
        TickService m_TickService;
        FlatService m_FlatService{};
        PipelineService m_PipelineService;

        // Removes the published tick and the flattened ones.
        auto Cleanup() -> void
        {
            m_TickStore.Cleanup();
            m_FlatStore.Cleanup();
        }
    };
}