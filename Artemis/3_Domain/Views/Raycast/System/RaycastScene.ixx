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
    using Views::Raycast::Type::FixedPartsCache;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneKeyFunction;
    using Views::Raycast::Type::SceneObjects;
    using Views::Raycast::Type::ScenePartsFunction;
    using Views::Raycast::Type::SceneRefs;
    using Views::Raycast::Type::SelfExclusion;

    using Perception::Collidable::System::CollidableService;
    using Blam::Runtime::Object::State::ObjectTableStore;
    using Blam::MapModel::World::System::Raycaster;
}

export namespace Views::Raycast::System
{
    // Collects the parts of every object of the tick that has collision, sorted by handle.
    // param reuse: The parts of the fixed objects of the last gather. A fixed object whose key is the same keeps its parts without
    // collecting them again, and an object that is gone or no longer fixed leaves it (D299).
    // param reused: Receives how many objects kept their parts. Null for none.
    // note: No radius: it is the whole tick. Objects with Fixed or Phased placement are marked as fixed. The result is the one of
    // collecting every object.
    auto GatherSceneObjects(FixedPartsCache& reuse, CollidableService& collidableService, const ObjectTableStore& objectStore,
        std::size_t* reused) -> SceneObjects;

    // The same gather over the given objects, with the key and the parts of an object given by functions.
    auto GatherSceneObjects(FixedPartsCache& reuse, const SceneRefs& refs, const SceneKeyFunction& keyOf, const ScenePartsFunction& partsOf,
        std::size_t* reused) -> SceneObjects;

    // Builds the hierarchies of the collision meshes of the objects that do not have one yet.
    // return: How many it built.
    auto PrepareMeshBvhs(RaycastScene& scene, const SceneObjects& objects) -> std::size_t;

    // Rebuilds the fixed tree only if the set of fixed parts or any of their transforms is not exactly what it was built from.
    // return: Whether it rebuilt it.
    auto ArmFixedScene(RaycastScene& scene, const SceneObjects& objects) -> bool;

    // Rebuilds the tree of the objects that are not fixed. Runs every decision tick.
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