export module Viewer.Geometry.System;

import Platform.Render.Type;
import Common.Math.Type;
import Common.ZoneShape.Type;
import Viewer.Style.Type;
import std;

export namespace Viewer::Geometry::System::ZoneGeometry
{
    using Vertex = Platform::Render::Type::Vertex;
    using Vec3 = Common::Math::Type::Vec3;
    using ZoneShape = Common::ZoneShape::Type::ZoneShape;
    using Color = Viewer::Style::Type::Color;

    // Appends the triangles of a zone as a closed solid. The zone is a cylinder or a box.
    // param forward: With up, defines the orientation of the zone.
    // param zone: Zones of another kind add nothing.
    auto AppendSolid(std::vector<Vertex>& out, const Vec3& position,
        const Vec3& forward, const Vec3& up, const ZoneShape& zone,
        const Color& color) -> void;
}