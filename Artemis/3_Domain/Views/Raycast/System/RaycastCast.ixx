export module Views.Raycast.System:Cast;

import Common.Math.Type;
import Blam.MapModel.World.System;
import Blam.MapModel.Definitions.State;
import Blam.Runtime.Player.State;
import Blam.Runtime.Object.State;
import Perception.Collidable.Type;
import Perception.Collidable.System;
import Perception.Entities.State;
import Perception.Players.State;
import Views.Raycast.Type;
import std;

// The pieces of the rays of one agent as free functions that take the stores and services they read (D273). RaycastService composes
// them, and the offline bench of the rays (Tests) measures each one apart. Internal to Views: not in the public interface.

namespace
{
    using Common::Math::Type::Vec3;
    using Perception::Collidable::Type::CollidablePart;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::DynamicCache;
    using Views::Raycast::Type::PerceptionOrigin;
    using Views::Raycast::Type::RaycastHit;
    using Views::Raycast::Type::SelfExclusion;
    using Views::Raycast::Type::Constant::k_PerceptionRayCount;

    using Perception::Collidable::System::CollidableService;
    using Blam::MapModel::Definitions::State::DefinitionsStore;
    using Perception::Entities::State::EntitiesStore;
    using Blam::Runtime::Object::State::ObjectTableStore;
    using Perception::Players::State::PlayersStore;
    using Blam::Runtime::Player::State::PlayerTableStore;
    using Blam::MapModel::World::System::Raycaster;
}

export namespace Views::Raycast::System
{
    // return: The world directions of the perception rays (a cube: six faces and eight corners) for an agent that looks at forward.
    auto PerceptionDirections(const Vec3& forward) -> std::array<Vec3, k_PerceptionRayCount>;

    // return: The radius of the dynamic cache of one agent: the farthest ray, measured from the camera.
    auto DynamicCacheRadius(float aimRange, float cameraToOriginDistance) -> float;

    // return: The aim range of the weapon of the player, capped at the maximum ray range. The fallback range if it has none.
    auto ResolveAimRange(const PlayerTableStore& playerStore, const ObjectTableStore& objectStore,
        const DefinitionsStore& definitionsStore, std::uint32_t selfPlayerHandle) -> float;

    // Collects the handles the rays of the agent ignore: its biped, its loadout and the vehicle it rides with its parts.
    auto BuildSelfExclusion(const PlayersStore& playersStore, const EntitiesStore& entitiesStore,
        const ObjectTableStore& objectStore, std::uint32_t selfPlayerHandle, std::uint32_t selfBipedHandle) -> SelfExclusion;

    // Returns the center and the forward of the biped, or of its vehicle when it rides one.
    auto ResolvePerceptionOrigin(const EntitiesStore& entitiesStore, const ObjectTableStore& objectStore,
        std::uint32_t selfBipedHandle) -> PerceptionOrigin;

    // Prepares the parts of an object to be hit by rays.
    // note: Computed once per tick for each object, instead of posing every triangle.
    auto PrepareParts(const std::vector<CollidablePart>& parts) -> std::vector<CachedPart>;

    // Builds the cache of the objects of the candidates that the exclusion does not hide.
    auto ArmDynamicCache(CollidableService& collidableService, const std::vector<std::uint32_t>& candidates,
        const SelfExclusion& selfExclusion) -> DynamicCache;

    // Builds the cache of the dynamic objects within a radius of the origin: QueryNearby and ArmDynamicCache.
    auto BuildDynamicCache(CollidableService& collidableService, const Vec3& origin, float radius,
        const SelfExclusion& selfExclusion) -> DynamicCache;

    // Whether a ray passes within a radius of a point, before travelling limit.
    auto RayNearPoint(const Vec3& origin, const Vec3& direction, float limit, const Vec3& center, float radius) -> bool;

    // Whether a ray crosses a box before travelling tMax.
    auto RayHitsBox(const Vec3& origin, const Vec3& direction, const Vec3& boxMin, const Vec3& boxMax, float tMax) -> bool;

    // return: The hit of the ray against the static world, with the origin, direction and range of the ray filled in.
    auto CastStatic(const Raycaster& raycaster, const Vec3& origin, const Vec3& direction, float maxDistance) -> RaycastHit;

    // Looks for a dynamic hit nearer than the one in best, and replaces it.
    // param best: The result of CastStatic for the same ray.
    auto CastDynamic(const DynamicCache& dynamicCache, const Vec3& origin, const Vec3& direction, float maxDistance,
        RaycastHit& best) -> void;

    // return: The nearest hit of the ray against the static world and the dynamic cache.
    auto CastRay(const Raycaster& raycaster, const DynamicCache& dynamicCache, const Vec3& origin, const Vec3& direction,
        float maxDistance) -> RaycastHit;
}