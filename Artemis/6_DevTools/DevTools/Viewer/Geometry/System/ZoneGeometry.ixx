export module Viewer.Geometry.System;

import Common.Math.Type;
import Common.ZoneShape.Type;
import Platform.Render.Type;
import Viewer.Style.Type;
import std;

namespace
{
    using Platform::Render::Type::Vertex;
    using Common::Math::Type::Vec3;
    using Common::ZoneShape::Type::ZoneShape;
    using Viewer::Style::Type::Color;
}

export namespace Viewer::Geometry::System
{
    // Appends the triangles of a zone as a closed solid. The zone is a cylinder or a box.
    // param forward: With up, defines the orientation of the zone.
    // param zone: Zones of another kind add nothing.
    auto AppendSolid(std::vector<Vertex>& out, const Vec3& position,
        const Vec3& forward, const Vec3& up, const ZoneShape& zone,
        const Color& color) -> void;
}