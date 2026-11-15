export module Views.Raycast.System:Scene;

import Common.Math.Type;
import Blam.MapModel.World.System;
import Blam.Runtime.Object.State;
import Perception.Collidable.System;
import Views.Raycast.Type;
import std;

// The two level ray scene that all agents share (D298): one hierarchy per collision mesh and two hierarchies of instances, one for
// the fixed objects and one for the rest. Free functions over data of the Type module, so whoever owns the RaycastScene decides when
// it lives. Not wired into the pipeline yet and internal to Views: not in the public interface.

namespace
{
    using Common::Math::Type::Vec3;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::SceneCounters;
    using Views::Raycast::Type::SceneObjects;
    using Views::Raycast::Type::SelfExclusion;

    using Perception::Collidable::System::CollidableService;
    using Blam::Runtime::Object::State::ObjectTableStore;
    using Blam::MapModel::World::System::Raycaster;
}

export namespace Views::Raycast::System
{
    // Collects the parts of every object of the tick that has collision, sorted by handle.
    // note: No radius: it is the whole tick. Objects with Fixed or Phased placement are marked as fixed.
    auto GatherSceneObjects(CollidableService& collidableService, const ObjectTableStore& objectStore) -> SceneObjects;

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

    // Forgets the hierarchies, with the map.
    auto CleanupScene(RaycastScene& scene) -> void;
}