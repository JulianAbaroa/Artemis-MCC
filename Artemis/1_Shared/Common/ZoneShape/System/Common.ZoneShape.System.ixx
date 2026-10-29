export module Common.ZoneShape.System;

import Common.Math.Type;
import Common.ZoneShape.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::ZoneShape::Type::ZoneShape;
}

export namespace Common::ZoneShape::System
{
    // Checks whether a point is inside a zone, with the convention the viewer draws (Right = Forward x Up).
    // param origin: WorldOrigin of the object that carries the zone.
    // param forward: Forward of that object. Together with up it gives the axes the shape is measured along.
    // note: A cylinder is round in the Right and Forward plane with the radius, and goes from -Bottom to +Top along Up. A box has
    // a width of Radius along Right, Length along Forward and the same height. A zone of kind None contains nothing. The border counts as inside.
    auto Contains(const ::ZoneShape& shape, const Vec3& origin, const Vec3& forward, const Vec3& up, const Vec3& point) -> bool;
}