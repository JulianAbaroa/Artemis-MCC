module Common.ZoneShape.System;

import Common.Math.System;
import std;

namespace
{
    namespace VecMath = Common::Math::System;

    using Common::Math::Type::Vec3;
    using Common::ZoneShape::Type::ZoneShape;
    using Common::ZoneShape::Type::ZoneShapeKind;
}

namespace Common::ZoneShape::System
{
    auto Contains(const ::ZoneShape& shape, const Vec3& origin, const Vec3& forward, const Vec3& up, const Vec3& point) -> bool
    {
        if (shape.Kind != ZoneShapeKind::Cylinder && shape.Kind != ZoneShapeKind::Box) return false;

        // The same orthonormal basis the viewer draws the zone with: Up is corrected to be perpendicular to Forward.
        const Vec3 basisForward{ VecMath::Normalize(forward) };
        const Vec3 basisRight{ VecMath::Normalize(VecMath::Cross(basisForward, VecMath::Normalize(up))) };
        const Vec3 basisUp{ VecMath::Normalize(VecMath::Cross(basisRight, basisForward)) };

        const Vec3 offset{ VecMath::Subtract(point, origin) };
        const float right{ VecMath::Dot(offset, basisRight) };
        const float ahead{ VecMath::Dot(offset, basisForward) };
        const float height{ VecMath::Dot(offset, basisUp) };

        if (height > shape.Top || height < -shape.Bottom) return false;

        if (shape.Kind == ZoneShapeKind::Cylinder) return (right * right) + (ahead * ahead) <= shape.Radius * shape.Radius;

        return std::abs(right) <= shape.Radius * 0.5f && std::abs(ahead) <= shape.Length * 0.5f;
    }
}