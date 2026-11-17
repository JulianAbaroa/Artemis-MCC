export module Views.Raycast.System:Scene;

import Common.Math.Type;
import Blam.MapModel.World.System;
import Blam.Runtime.Object.State;
import Perception.Collidable.System;
import Views.Raycast.Type;
import std;

// The two level ray scene that all agents share (D298): one hierarchy per collision mesh and two hierarchies of instances, one for
// the fixed objects and one for the rest. Free functions over data of the Type module, so whoever owns the RaycastScene decides when
// it lives (RaycastSceneStore). Internal to Views: not in the public interface.

namespace
{
    using Common::Math::Type::Vec3;
    using Views::Raycast::Type::FixedRebuildRule;
    using Views::Raycast::Type::MobileRebuildRule;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneReach;
    using Views::Raycast::Type::SceneGatherCounters;
    using Views::Raycast::Type::SceneKeyFunction;
    using Views::Raycast::Type::SceneObjects;
    using Views::Raycast::Type::ScenePartsCache;
    using Views::Raycast::Type::ScenePartsFunction;
    using Views::Raycast::Type::SceneRefs;
    using Views::Raycast::Type::SelfExclusion;

    using Perception::Collidable::System::CollidableService;
    using Blam::Runtime::Object::State::ObjectTableStore;
    using Blam::MapModel::World::System::Raycaster;
}

export namespace Views::Raycast::System
{
    // Gathers the objects of the tick and arms the fixed and the mobile tree, once for all the agents of the tick (F4.2).
    // param parts: The parts of the objects of the last gather.
    // note: It marks the scene as prepared for the object table of the tick and counts the call in RaycastScene::Prepares.
    auto PrepareScene(RaycastScene& scene, ScenePartsCache& parts, CollidableService& collidableService, const ObjectTableStore& objectStore) -> void;

    // return: Whether the scene was prepared for the object table that is published now, by a PrepareScene since the last call of this function.
    // note: Takes the mark: the next call answers false until the scene is prepared again. The Self of before uses it to share the gather of
    // the Views in a tick and to gather by itself when it runs alone (F4.2, until the Self is deleted).
    auto TakePreparedScene(RaycastScene& scene, const ObjectTableStore& objectStore) -> bool;

    // Collects the parts of every object of the tick that has collision, sorted by handle, and marks the ones at rest.
    // param reuse: The parts of the objects of the last gather. An object whose key is the same keeps its parts without collecting them
    // again, and an object that is gone leaves it (D299, D300).
    // param counters: Receives what the gather did. Null for none.
    // note: No radius: it is the whole tick. An object is at rest (IsFixed) if its placement is Fixed or Phased and its key did not change
    // since the last gather or it is new, or if its key was the same in the last k_RestGathers gathers. A change of key takes it out of
    // rest in that same gather. The parts are the ones of collecting every object.
    auto GatherSceneObjects(ScenePartsCache& reuse, CollidableService& collidableService, const ObjectTableStore& objectStore,
        SceneGatherCounters* counters) -> SceneObjects;

    // The same gather over the given objects, with the key and the parts of an object given by functions.
    // note: The key of each object is updated in its entry of reuse, in place. The parts of a SceneObject are a view of that entry, so
    // they are valid until the next gather: arm and cast before gathering again. An entry is replaced only for objects whose key changed,
    // before their SceneObject is made, and only entries of objects not in the result are erased.
    auto GatherSceneObjects(ScenePartsCache& reuse, const SceneRefs& refs, const SceneKeyFunction& keyOf, const ScenePartsFunction& partsOf,
        SceneGatherCounters* counters) -> SceneObjects;

    // Builds the hierarchies of the collision meshes of the objects whose IsCovered is covered that do not have one yet.
    // note: The arming functions call it for the objects of the tree they build, so the objects the other tree keeps are not walked.
    // return: How many it built.
    auto PrepareMeshBvhs(RaycastScene& scene, const SceneObjects& objects, bool covered) -> std::size_t;

    // Keeps the fixed tree up to date with the objects at rest and marks which objects it covers (D300).
    // param objects: Sorted by handle, as GatherSceneObjects gives them. Their IsCovered is set.
    // return: Whether it built the fixed tree.
    // note: An object in the tree that is not at rest with the same poses any more goes to the filter of the scene: its old parts stop
    // hitting at once and its current ones go to the mobile tree. An object at rest that is not in the tree waits in the mobile tree.
    // The tree is built again, with every object at rest and an empty filter, when something waits (objects at rest not in it plus
    // filtered handles) and either it was never built, or rule.Spacing gathers passed since the last build and there are rule.Pending
    // waiting or the oldest wait lasted rule.MaxGathers gathers (D301). What waits lives in the mobile tree and the filter, so the hits
    // do not depend on when the tree is built.
    auto ArmFixedScene(RaycastScene& scene, SceneObjects& objects, const FixedRebuildRule& rule = {}) -> bool;

    // Keeps the tree of the objects that the fixed tree does not cover up to date. Runs every decision tick, after ArmFixedScene (D302).
    // note: Its pieces are the parts of those objects in their pose of the tick. When they are the same pieces as in the last arming
    // (same handles, meshes and part indices, in the same order) only their poses changed: the hierarchy keeps its topology and its boxes
    // are refitted from the leaves up, which holds exactly what a build would. The tree is built again when the pieces change (an object
    // appears, leaves or changes its number of parts), or when the refit ends with the area of its inner nodes above rule.MaxArea times
    // the area it had when it was built. The hits do not depend on which of the two ran. RaycastSceneStore::Cleanup drops the state with the scene.
    auto ArmMobileScene(RaycastScene& scene, const SceneObjects& objects, const MobileRebuildRule& rule = {}) -> void;

    // Looks for a dynamic hit nearer than the one in best through both trees, and replaces it.
    // param best: The result of CastStatic for the same ray.
    // param exclusion: Handles the ray ignores.
    // param counters: Receives what the cast visited. Null for none.
    // note: At equal distance a static hit stays, and between dynamic hits the lowest handle, then part, then triangle wins. A ray whose origin,
    // direction or range is not a number (NaN or infinite) leaves best as it is (RV.1).
    auto CastScene(const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin, const Vec3& direction,
        float maxDistance, RaycastHit& best, SceneCounters* counters) -> void;

    // Gathers the pieces of both trees that a ray from origin of at most radius can touch, once for the rays of an agent that share their origin (RK.1).
    // param exclusion: Handles the rays ignore: their pieces are left out here, so the casts do not look at them.
    // param counters: Receives the nodes of the trees the gather visited (TreeNodes) and the pieces it kept (Pieces). Null for none.
    // note: A piece that a ray from the origin within the radius touches is always in it: the nodes are tested against the box of the ball and
    // the pieces against its sphere, and both hold anything such a ray can hit. The pieces of the fixed tree that the filter lists are left out.
    auto GatherSceneReach(const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin, float radius, SceneCounters* counters) -> SceneReach;

    // return: The nearest hit of the ray against the static world and the scene, through the pieces of reach.
    // note: The same hit as CastSceneRay. A ray that does not start at the origin of reach, is longer than its radius, has another exclusion than the
    // one the reach was gathered with, or comes after the scene was armed again, is cast through the trees (RV.1).
    auto CastSceneRayReach(const Raycaster& raycaster, const RaycastScene& scene, const SceneReach& reach, const SelfExclusion& exclusion,
        const Vec3& origin, const Vec3& direction, float maxDistance, SceneCounters* counters) -> RaycastHit;

    // return: The nearest hit of the ray against the static world and the scene.
    auto CastSceneRay(const Raycaster& raycaster, const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin,
        const Vec3& direction, float maxDistance, SceneCounters* counters) -> RaycastHit;

}