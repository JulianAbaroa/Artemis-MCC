export module Views.Raycast.System:Cast;

import Common.Math.Type;
import Blam.MapModel.World.System;
import Perception.Collidable.Type;
import Views.Raycast.Type;
import std;

// The pieces of the rays of one agent as free functions (D273). ViewsService composes them, and the offline bench of the rays (Tests)
// measures each one apart. Internal to Views: not in the public interface.

namespace
{
    using Common::Math::Type::Vec3;
    using Perception::Collidable::Type::CollidablePart;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::RaycastHit;

    using Blam::MapModel::World::System::Raycaster;
}

export namespace Views::Raycast::System
{
    // Prepares one part to be hit by rays.
    // note: Computed once per tick for each part, instead of posing every triangle.
    // param out: Receives the part when it can be hit. Every field is written.
    // return: False for a part without a collision mesh, with no triangles or with a transform that cannot be inverted: it is left out.
    auto PreparePart(const CollidablePart& part, CachedPart& out) -> bool;

    // Whether a ray passes within a radius of a point, before travelling limit.
    auto RayNearPoint(const Vec3& origin, const Vec3& direction, float limit, const Vec3& center, float radius) -> bool;

    // Whether a ray crosses a box before travelling tMax.
    // param inverse: InverseDirection of each component of the direction of the ray.
    auto RayHitsBox(const Vec3& origin, const std::array<float, 3>& inverse, const Vec3& boxMin, const Vec3& boxMax, float tMax) -> bool;

    // Takes a ray to the local space of a part.
    // param toLocal: The ToLocal of a CachedPart.
    // note: The direction is not normalized again, so distances along the local ray are the same as along the world ray.
    auto PoseRay(const std::array<float, 12>& toLocal, const Vec3& origin, const Vec3& direction, Vec3& localOrigin,
        Vec3& localDirection) -> void;

    // return: The hit of the ray against the static world, with the origin, direction and range of the ray filled in.
    auto CastStatic(const Raycaster& raycaster, const Vec3& origin, const Vec3& direction, float maxDistance) -> RaycastHit;
}