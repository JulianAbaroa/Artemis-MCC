export module Views.Layer;

import Common.Log.System;
import Blam.MapModel.Layer;
import Blam.Runtime.Layer;
import Perception.Layer;
import Views.Self.State;
import Views.Self.System;
import Views.Affordance.State;
import Views.Affordance.System;
import Views.Raycast.State;
import Views.Raycast.System;

export namespace Views
{
    // Builds the view of each local player: its self, the affordances and the raycasts.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using SelfStore = Views::Self::State::SelfStore;
        using AffordanceStore = Views::Affordance::State::AffordanceStore;
        using RaycastStore = Views::Raycast::State::RaycastStore;

        using SelfService = Views::Self::System::SelfService;
        using AffordanceService = Views::Affordance::System::AffordanceService;
        using RaycastService = Views::Raycast::System::RaycastService;

    public:
        Layer(Log& log, Blam::MapModel::Layer& resolved, Blam::Runtime::Layer& tables,
            Perception::Layer& environment) :
            m_SelfService(log, tables.m_PlayerStore, m_SelfStore),
            m_AffordanceService(log, tables.m_ObjectStore, tables.m_BoneMatricesStore, tables.m_InteractionStore, m_SelfStore, environment.m_ClassifierStore, environment.m_ObjectGraphStore, m_AffordanceStore, environment.m_CollidableService),
            m_RaycastService(log, tables.m_PlayerStore, tables.m_ObjectStore, resolved.m_DefinitionsStore, resolved.m_Raycaster, m_SelfStore, environment.m_CollidableService, environment.m_EntitiesStore, environment.m_PlayersStore, m_RaycastStore) {}
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        SelfStore m_SelfStore{};
        AffordanceStore m_AffordanceStore{};
        RaycastStore m_RaycastStore{};

        // --- System ---
        SelfService m_SelfService;
        AffordanceService m_AffordanceService;
        RaycastService m_RaycastService;

        // Removes the self, affordances and raycasts.
        auto Cleanup() -> void
        {
            m_SelfService.Cleanup();
            m_AffordanceService.Cleanup();
            m_RaycastService.Cleanup();
        }
    };
}