export module Views.View.System;

import Common.Log.System;
import Common.Math.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.World.System;
import Blam.Runtime.Object.State;
import Perception.Collidable.System;
import Perception.Details.State;
import Perception.Entities.State;
import Perception.ObjectGraph.State;
import Perception.Players.State;
import Views.Raycast.Type;
import Views.Raycast.State;
import Views.View.Type;
import Views.View.State;
import std;

export namespace Views::View::System
{
    // return: The View of the player with that handle, or the View of the primary local player when the handle is 0xFFFFFFFF or has none.
    // Null when neither has a computed View. The Viewer and the oracle read the View of the player they watch through it (F4.2).
    auto FindObservedView(const Views::View::Type::ViewTable& views, std::uint32_t observedHandle) -> const Views::View::Type::View*;

    // return: The unit direction the agent aims along, from the heading and pitch of its aim.
    auto AimDirectionOf(const Views::View::Type::View& view) -> Common::Math::Type::Vec3;

    // return: Where the agent looks from: its eye, or its feet when the biped has no tag (the eye is zero then).
    auto ViewPositionOf(const Views::View::Type::View& view) -> Common::Math::Type::Vec3;

    // return: The raycasts of a View in the form the dev tools draw and compare: the aim ray from the eye along the aim, and the rays of the
    // pattern from the ray origin in the frame of the aim. Empty perception hits when the rays were not cast (dead agent, no body).
    auto RaycastsOf(const Views::View::Type::View& view) -> Views::Raycast::Type::Raycasts;

    // Builds the View of every agent of the set, one per row of Players (F4, D307).
    // note: F4.1 fills the head and Self, F4.2 the aim hit and the rays. The other sections are zero until F4.3 to F4.6.
    class ViewsService
    {
    private:
        using Log = Common::Log::System::Log;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;
        using ObjectiveDetailsStore = Perception::Details::State::ObjectiveDetailsStore;
        using ObjectGraphStore = Perception::ObjectGraph::State::ObjectGraphStore;
        using CollidableService = Perception::Collidable::System::CollidableService;
        using Raycaster = Blam::MapModel::World::System::Raycaster;
        using RaycastSceneStore = Views::Raycast::State::RaycastSceneStore;
        using WeaponDetailsStore = Perception::Details::State::WeaponDetailsStore;
        using EntitiesStore = Perception::Entities::State::EntitiesStore;
        using PlayersStore = Perception::Players::State::PlayersStore;
        using ViewsStore = Views::View::State::ViewsStore;
        using AgentSet = Views::View::Type::AgentSet;

    public:
        ViewsService(Log& log, ObjectTableStore& objectStore, DefinitionsStore& definitionsStore,
            EntitiesStore& entitiesStore, PlayersStore& playersStore, WeaponDetailsStore& weaponDetailsStore,
            ObjectiveDetailsStore& objectiveDetailsStore, ObjectGraphStore& graphStore, Raycaster& raycaster,
            CollidableService& collidableService, RaycastSceneStore& sceneStore, ViewsStore& viewsStore) : m_Log(log),
            m_ObjectStore(objectStore), m_DefinitionsStore(definitionsStore), m_EntitiesStore(entitiesStore),
            m_PlayersStore(playersStore), m_WeaponDetailsStore(weaponDetailsStore),
            m_ObjectiveDetailsStore(objectiveDetailsStore), m_GraphStore(graphStore), m_Raycaster(raycaster),
            m_CollidableService(collidableService), m_SceneStore(sceneStore), m_ViewsStore(viewsStore) {}
        ~ViewsService() = default;

        // Publishes the Views of the tick: a computed one for each player of the set, a zero one for the others.
        // note: Reads the stores the same tick published, so it runs after Perception. The ray scene is prepared once, when the first agent
        // with a body needs it, and the exclusions of every player come from one pass over the object graph (F4.2).
        auto Update(const AgentSet& agents) -> void;

        auto Cleanup() -> void;

    private:
        Log& m_Log;
        ObjectTableStore& m_ObjectStore;
        DefinitionsStore& m_DefinitionsStore;
        EntitiesStore& m_EntitiesStore;
        PlayersStore& m_PlayersStore;
        WeaponDetailsStore& m_WeaponDetailsStore;
        ObjectiveDetailsStore& m_ObjectiveDetailsStore;
        ObjectGraphStore& m_GraphStore;
        Raycaster& m_Raycaster;
        CollidableService& m_CollidableService;
        RaycastSceneStore& m_SceneStore;
        ViewsStore& m_ViewsStore;
    };
}