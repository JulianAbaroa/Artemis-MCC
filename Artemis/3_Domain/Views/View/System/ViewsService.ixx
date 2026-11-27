export module Views.View.System;

import Common.Log.System;
import Common.Math.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.State;
import Blam.MapModel.World.System;
import Blam.Runtime.Interaction.State;
import Blam.Runtime.Object.State;
import Perception.Collidable.System;
import Perception.Details.Type;
import Perception.Details.State;
import Perception.Entities.State;
import Perception.ObjectGraph.State;
import Perception.Players.Type;
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

    // return: The handle of the player whose current biped is the selected object, or 0xFFFFFFFF (the primary local player) when the selection is
    // not a player biped or there is none. It is the observed player of the Viewer: selecting the biped of a bot shows its View without moving the camera.
    auto ObservedPlayerOf(const Perception::Players::Type::Players& players, std::uint32_t selected) -> std::uint32_t;

    // return: The players that get a View in a tick, from who owns the instance (D307, D385). A Python client that owns it chooses with its mask. Without one,
    // every player when the Viewer wants all the Views (it has no focus and its layers draw each agent), else none. The caller adds the players it always needs.
    auto AgentSetOf(bool isClientOwner, std::uint32_t clientMask, bool isAllViewsWanted) -> Views::View::Type::AgentSet;

    // return: The unit direction the agent aims along, from the heading and pitch of its aim.
    auto AimDirectionOf(const Views::View::Type::View& view) -> Common::Math::Type::Vec3;

    // return: Where the agent looks from: its eye, or its feet when the biped has no tag (the eye is zero then).
    auto ViewPositionOf(const Views::View::Type::View& view) -> Common::Math::Type::Vec3;

    // return: Where the two arrows of the observation of the agent go: from its feet to the enemy flag (Flags[1], the carrier's position while it is
    // carried) and to its own stand (Flags[0]). Nothing without a computed View or a body. A team with no side has neither, the enemy flag needs
    // HasFlag, and the stand a position other than zero.
    // note: Mirror of ObserveBatch in Agent/artemis/env/ctf.py (entries 7-10 and 14-17): change both together.
    auto ObsTargetsOf(const Views::View::Type::View& view) -> Views::View::Type::ObsTargets;

    // return: The end of an observation arrow as the Viewer draws it (VW.17, D387). With a length of zero or less, or a target that is not farther than the
    // length, the target. Else the point at that length from the start along the direction of the target.
    auto ObsArrowEnd(const Common::Math::Type::Vec3& from, const Common::Math::Type::Vec3& target, float length) -> Common::Math::Type::Vec3;

    // return: The team that owns the rival stand of the team, the one the agent's enemy flag belongs to (the lowest stand handle if several, as
    // Flags[1] is chosen). Neutral for a team without a side or a game without a rival stand.
    auto RivalTeamOf(const Perception::Details::Type::ObjectiveDetails& objectives, Common::Team::Type::Team team) -> Common::Team::Type::Team;

    // return: The raycasts of a View in the form the dev tools draw and compare: the aim ray from the eye along the aim, and the rays of the
    // pattern from the ray origin in the frame of the aim. Empty perception hits when the rays were not cast (dead agent, no body).
    auto RaycastsOf(const Views::View::Type::View& view) -> Views::Raycast::Type::Raycasts;

    // Builds the View of every agent of the set, one per row of Players (F4, D307).
    // note: F5.1 A also fills Interaction from the selections of the interaction store (the action applied to the agent, D371).
    // F4.1 fills the head and Self, F4.2 the aim hit and the rays. The other sections are zero until F4.3 to F4.6.
    class ViewsService
    {
    private:
        using Log = Common::Log::System::Log;
        using DefinitionsStore = Blam::MapModel::Definitions::State::DefinitionsStore;
        using InteractionStore = Blam::Runtime::Interaction::State::InteractionStore;
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
        ViewsService(Log& log, ObjectTableStore& objectStore, InteractionStore& interactionStore, DefinitionsStore& definitionsStore,
            EntitiesStore& entitiesStore, PlayersStore& playersStore, WeaponDetailsStore& weaponDetailsStore,
            ObjectiveDetailsStore& objectiveDetailsStore, ObjectGraphStore& graphStore, Raycaster& raycaster,
            CollidableService& collidableService, RaycastSceneStore& sceneStore, ViewsStore& viewsStore) : m_Log(log),
            m_ObjectStore(objectStore), m_InteractionStore(interactionStore), m_DefinitionsStore(definitionsStore), m_EntitiesStore(entitiesStore),
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
        InteractionStore& m_InteractionStore;
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