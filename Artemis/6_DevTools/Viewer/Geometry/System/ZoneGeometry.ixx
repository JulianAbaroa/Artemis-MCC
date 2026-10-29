export module DevTools.Viewer.Geometry.System;

import Common.Math.Type;
import Common.ZoneShape.Type;
import Engine.Render.Type;
import DevTools.Viewer.Style.Type;
import std;

namespace
{
    using Engine::Render::Type::Vertex;
    using Common::Math::Type::Vec3;
    using Common::ZoneShape::Type::ZoneShape;
    using DevTools::Viewer::Style::Type::Color;
}

export namespace DevTools::Viewer::Geometry::System
{
    // Appends the triangles of a zone as a closed solid. The zone is a cylinder or a box.
    // param forward: With up, defines the orientation of the zone.
    // param zone: Zones of another kind add nothing.
    auto AppendSolid(std::vector<Vertex>& out, const Vec3& position,
        const Vec3& forward, const Vec3& up, const ZoneShape& zone,
        const Color& color) -> void;
}