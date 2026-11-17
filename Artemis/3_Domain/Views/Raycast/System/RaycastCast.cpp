module;

#include <cmath>

module Views.Raycast.System;

import Blam.MapModel.World.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Views::Raycast::Type::CachedPart;
    using Views::Raycast::Type::HitKind;
    using Views::Raycast::Type::RaycastHit;
    using Perception::Collidable::Type::CollidablePart;

    using Blam::MapModel::World::System::Raycaster;
    using Blam::MapModel::World::Type::Raycast::k_BoxPadding;
}

namespace Views::Raycast::System
{
    auto PreparePart(const CollidablePart& part, CachedPart& out) -> bool
    {
        if (!part.Source || part.Source->Triangles.empty()) return false;

        const auto& m = part.Transform;

        const float a = m[0], b = m[1], c = m[2];
        const float d = m[4], e = m[5], f = m[6];
        const float g = m[8], h = m[9], i = m[10];

        const float det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
        if (std::fabs(det) < 1.0e-8f) return false;
        const float inv = 1.0f / det;

        const float i00 = (e * i - f * h) * inv;
        const float i01 = (c * h - b * i) * inv;
        const float i02 = (b * f - c * e) * inv;
        const float i10 = (f * g - d * i) * inv;
        const float i11 = (a * i - c * g) * inv;
        const float i12 = (c * d - a * f) * inv;
        const float i20 = (d * h - e * g) * inv;
        const float i21 = (b * g - a * h) * inv;
        const float i22 = (a * e - b * d) * inv;

        out.Source = part.Source;
        out.ToLocal = {
            i00, i01, i02, -(i00 * m[3] + i01 * m[7] + i02 * m[11]),
            i10, i11, i12, -(i10 * m[3] + i11 * m[7] + i12 * m[11]),
            i20, i21, i22, -(i20 * m[3] + i21 * m[7] + i22 * m[11])
        };

        // The box is posed through its eight corners, so the sphere holds for any transform.
        const auto& lo = part.Source->LocalMin;
        const auto& hi = part.Source->LocalMax;
        auto toWorld = [&](float x, float y, float z) -> Common::Math::Type::Vec3 {
            return {
                m[0] * x + m[1] * y + m[2] * z + m[3],
                m[4] * x + m[5] * y + m[6] * z + m[7],
                m[8] * x + m[9] * y + m[10] * z + m[11]
            };
        };

        out.Center = toWorld((lo.X + hi.X) * 0.5f, (lo.Y + hi.Y) * 0.5f, (lo.Z + hi.Z) * 0.5f);

        float radiusSq{};
        Common::Math::Type::Vec3 boxMin{ out.Center };
        Common::Math::Type::Vec3 boxMax{ out.Center };
        for (const float x : { lo.X, hi.X })
        {
            for (const float y : { lo.Y, hi.Y })
            {
                for (const float z : { lo.Z, hi.Z })
                {
                    const auto corner = toWorld(x, y, z);
                    const float dx = corner.X - out.Center.X;
                    const float dy = corner.Y - out.Center.Y;
                    const float dz = corner.Z - out.Center.Z;
                    radiusSq = (std::max)(radiusSq, dx * dx + dy * dy + dz * dz);
                    boxMin = { (std::min)(boxMin.X, corner.X), (std::min)(boxMin.Y, corner.Y), (std::min)(boxMin.Z, corner.Z) };
                    boxMax = { (std::max)(boxMax.X, corner.X), (std::max)(boxMax.Y, corner.Y), (std::max)(boxMax.Z, corner.Z) };
                }
            }
        }

        out.Radius = std::sqrt(radiusSq);
        out.Min = boxMin;
        out.Max = boxMax;
        return true;
    }

    auto RayNearPoint(const Common::Math::Type::Vec3& origin, const Common::Math::Type::Vec3& direction,
        float limit, const Common::Math::Type::Vec3& center, float radius) -> bool
    {
        const float px = center.X - origin.X;
        const float py = center.Y - origin.Y;
        const float pz = center.Z - origin.Z;

        float along = px * direction.X + py * direction.Y + pz * direction.Z;
        along = (std::max)(0.0f, (std::min)(along, limit));

        const float cx = px - direction.X * along;
        const float cy = py - direction.Y * along;
        const float cz = pz - direction.Z * along;

        return cx * cx + cy * cy + cz * cz <= radius * radius;
    }

    auto RayHitsBox(const Common::Math::Type::Vec3& origin, const std::array<float, 3>& inverse,
        const Common::Math::Type::Vec3& boxMin, const Common::Math::Type::Vec3& boxMax, float tMax) -> bool
    {
        const std::array<float, 3> o{ origin.X, origin.Y, origin.Z };
        const std::array<float, 3> lo{ boxMin.X - k_BoxPadding, boxMin.Y - k_BoxPadding, boxMin.Z - k_BoxPadding };
        const std::array<float, 3> hi{ boxMax.X + k_BoxPadding, boxMax.Y + k_BoxPadding, boxMax.Z + k_BoxPadding };

        float t0{ 0.0f };
        float t1{ tMax };
        for (int axis = 0; axis < 3; ++axis)
        {
            const float ta = (lo[axis] - o[axis]) * inverse[axis];
            const float tb = (hi[axis] - o[axis]) * inverse[axis];
            t0 = (std::max)(t0, (std::min)(ta, tb));
            t1 = (std::min)(t1, (std::max)(ta, tb));
            if (t0 > t1) return false;
        }

        return true;
    }

    auto CastStatic(const Raycaster& raycaster, const Vec3& origin, const Vec3& direction, float maxDistance) -> RaycastHit
    {
        RaycastHit best{};
        best.Origin = origin;
        best.Direction = direction;
        best.MaxDistance = maxDistance;

        const auto staticHit = raycaster.Cast(origin, direction, maxDistance);
        if (staticHit.IsHit)
        {
            best.Hit = true;
            best.Kind = HitKind::Static;
            best.Distance = staticHit.Distance;
            best.Point = staticHit.Point;
            best.SurfaceFlags = staticHit.SurfaceFlags;
            best.Material = staticHit.Material;
        }

        return best;
    }

    auto PoseRay(const std::array<float, 12>& toLocal, const Vec3& origin, const Vec3& direction, Vec3& localOrigin,
        Vec3& localDirection) -> void
    {
        const auto& m = toLocal;
        localOrigin = {
            m[0] * origin.X + m[1] * origin.Y + m[2] * origin.Z + m[3],
            m[4] * origin.X + m[5] * origin.Y + m[6] * origin.Z + m[7],
            m[8] * origin.X + m[9] * origin.Y + m[10] * origin.Z + m[11]
        };
        localDirection = {
            m[0] * direction.X + m[1] * direction.Y + m[2] * direction.Z,
            m[4] * direction.X + m[5] * direction.Y + m[6] * direction.Z,
            m[8] * direction.X + m[9] * direction.Y + m[10] * direction.Z
        };
    }
}