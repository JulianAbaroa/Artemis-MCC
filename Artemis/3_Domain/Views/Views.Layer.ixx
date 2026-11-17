export module Views.Layer;

import Common.Log.System;
import Blam.MapModel.Layer;
import Blam.Runtime.Layer;
import Perception.Layer;
import Views.Affordance.State;
import Views.Affordance.System;
import Views.Raycast.State;
import Views.Raycast.System;
import Views.View.State;
import Views.View.System;

export namespace Views
{
    // Builds the affordances of the local player and the Views of the agents.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using AffordanceStore = Views::Affordance::State::AffordanceStore;
        using RaycastSceneStore = Views::Raycast::State::RaycastSceneStore;
        using ViewsStore = Views::View::State::ViewsStore;

        using AffordanceService = Views::Affordance::System::AffordanceService;
        using ViewsService = Views::View::System::ViewsService;

    public:
        Layer(Log& log, Blam::MapModel::Layer& resolved, Blam::Runtime::Layer& tables,
            Perception::Layer& environment) :
            m_AffordanceService(log, tables.m_ObjectStore, tables.m_BoneMatricesStore, tables.m_InteractionStore, tables.m_PlayerStore, environment.m_ClassifierStore, environment.m_ObjectGraphStore, m_AffordanceStore, environment.m_CollidableService),
            m_ViewsService(log, tables.m_ObjectStore, resolved.m_DefinitionsStore, environment.m_EntitiesStore, environment.m_PlayersStore, environment.m_WeaponDetailsStore, environment.m_ObjectiveDetailsStore, environment.m_ObjectGraphStore, resolved.m_Raycaster, environment.m_CollidableService, m_RaycastSceneStore, m_ViewsStore) {}
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // --- State ---
        AffordanceStore m_AffordanceStore{};
        RaycastSceneStore m_RaycastSceneStore{};
        ViewsStore m_ViewsStore{};

        // --- System ---
        AffordanceService m_AffordanceService;
        ViewsService m_ViewsService;

        // Removes the affordances and the Views.
        auto Cleanup() -> void
        {
            m_AffordanceService.Cleanup();
            m_ViewsService.Cleanup();
        }
    };
}