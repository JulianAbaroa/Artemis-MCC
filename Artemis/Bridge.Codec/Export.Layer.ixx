export module Export.Layer;

import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Tick.State;
import Export.Tick.System;
import Export.Flat.State;
import Export.Flat.System;
import Export.Pipeline.System;

export namespace Export
{
    // Packs what the other layers built into the Tick, and flattens it into the BridgeTick for the client.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using TickStore = Export::Tick::State::TickStore;
        using TickService = Export::Tick::System::TickService;
        using FlatStore = Export::Flat::State::FlatStore;
        using FlatService = Export::Flat::System::FlatService;
        using PipelineService = Export::Pipeline::System::PipelineService;

    public:
        Layer(Resolved::Layer& resolved, Tables::Layer& tables, Relations::Layer& relations,
            Environment::Layer& environment, Egocentric::Layer& egocentric) :
            m_TickService(tables.m_ObjectStore, tables.m_PlayerStore, tables.m_InteractionStore, relations.m_ClassifierStore, relations.m_ObjectGraphStore, environment.m_EntitiesStore, environment.m_PlayersStore, environment.m_VitalityDetailsStore, environment.m_ZoneDetailsStore, environment.m_TeleporterDetailsStore, environment.m_LiftDetailsStore, environment.m_BarrierDetailsStore, environment.m_AimStore, environment.m_EventsStore, environment.m_MatchStore, egocentric.m_SelfStore, egocentric.m_AffordanceStore, egocentric.m_RaycastStore, resolved.m_DefinitionsStore, m_TickStore),
            m_PipelineService(tables, relations, environment, egocentric, m_TickService) {}
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