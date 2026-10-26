export module Egocentric.Layer;

import Common.Log.System;
import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Self.State;
import Egocentric.Self.System;
import Egocentric.Affordance.State;
import Egocentric.Affordance.System;
import Egocentric.Raycast.State;
import Egocentric.Raycast.System;

export namespace Egocentric
{
    // Builds the view of each local player: its self, the affordances and the raycasts.
    // note: Pure: the owner connects the lifecycle (D178).
    class Layer
    {
    private:
        using Log = Common::Log::System::Log;

        using SelfStore = Egocentric::Self::State::SelfStore;
        using AffordanceStore = Egocentric::Affordance::State::AffordanceStore;
        using RaycastStore = Egocentric::Raycast::State::RaycastStore;

        using SelfService = Egocentric::Self::System::SelfService;
        using AffordanceService = Egocentric::Affordance::System::AffordanceService;
        using RaycastService = Egocentric::Raycast::System::RaycastService;

    public:
        Layer(Log& log, Resolved::Layer& resolved, Tables::Layer& tables, Relations::Layer& relations,
            Environment::Layer& environment) :
            m_SelfService(log, tables.m_PlayerStore, m_SelfStore),
            m_AffordanceService(log, tables.m_ObjectStore, tables.m_BoneMatricesStore, tables.m_InteractionStore, m_SelfStore, relations.m_ClassifierStore, relations.m_ObjectGraphStore, m_AffordanceStore, environment.m_CollidableService),
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