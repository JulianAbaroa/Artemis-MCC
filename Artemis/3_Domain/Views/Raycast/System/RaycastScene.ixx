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
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
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
    auto GatherSceneObjects(ScenePartsCache& reuse, const SceneRefs& refs, const SceneKeyFunction& keyOf, const ScenePartsFunction& partsOf,
        SceneGatherCounters* counters) -> SceneObjects;

    // Builds the hierarchies of the collision meshes of the objects that do not have one yet.
    // return: How many it built.
    auto PrepareMeshBvhs(RaycastScene& scene, const SceneObjects& objects) -> std::size_t;

    // Keeps the fixed tree up to date with the objects at rest and marks which objects it covers (D300).
    // param objects: Sorted by handle, as GatherSceneObjects gives them. Their IsCovered is set.
    // return: Whether it built the fixed tree.
    // note: An object in the tree that is not at rest with the same poses any more goes to the filter of the scene: its old parts stop
    // hitting at once and its current ones go to the mobile tree. An object at rest that is not in the tree waits in the mobile tree.
    // The tree is built again, with every object at rest and an empty filter, only if there is something waiting or filtered and
    // k_FixedRebuildGathers gathers passed since the last build, or it was never built.
    auto ArmFixedScene(RaycastScene& scene, SceneObjects& objects) -> bool;

    // Rebuilds the tree of the objects that the fixed tree does not cover. Runs every decision tick, after ArmFixedScene.
    auto ArmMobileScene(RaycastScene& scene, const SceneObjects& objects) -> void;

    // Looks for a dynamic hit nearer than the one in best through both trees, and replaces it.
    // param best: The result of CastStatic for the same ray.
    // param exclusion: Handles the ray ignores.
    // param counters: Receives what the cast visited. Null for none.
    // note: At equal distance a static hit stays, and between dynamic hits the lowest handle, then part, then triangle wins.
    auto CastScene(const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin, const Vec3& direction,
        float maxDistance, RaycastHit& best, SceneCounters* counters) -> void;

    // return: The nearest hit of the ray against the static world and the scene.
    auto CastSceneRay(const Raycaster& raycaster, const RaycastScene& scene, const SelfExclusion& exclusion, const Vec3& origin,
        const Vec3& direction, float maxDistance, SceneCounters* counters) -> RaycastHit;

}