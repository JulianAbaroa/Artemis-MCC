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

    // Appends one line segment (two vertices of a line list).
    auto AppendLine(std::vector<Vertex>& out, const Vec3& from, const Vec3& to, const Color& color) -> void;

    // Appends an arrow with a head at the tip. A dashed arrow is drawn as separate segments.
    // note: Draws nothing for a zero length arrow.
    auto AppendArrow(std::vector<Vertex>& out, const Vec3& from, const Vec3& to, const Color& color, bool dashed) -> void;

    // Appends the outline of a circle in the plane perpendicular to the normal, as a closed line loop.
    auto AppendCircle(std::vector<Vertex>& out, const Vec3& center, const Vec3& normal, float radius,
        int segments, const Color& color) -> void;

    // Appends a three-axis cross, the marker of a point.
    auto AppendCross(std::vector<Vertex>& out, const Vec3& center, float halfSize, const Color& color) -> void;

    // Appends the wire outline of a cone: a circle at its far end and some lines from the apex to it.
    // param slantLength: Length of each line from the apex, so the circle sits at slantLength * cos(halfAngle) along the axis.
    // param halfAngle: Radians.
    auto AppendCone(std::vector<Vertex>& out, const Vec3& apex, const Vec3& axis, float halfAngle, float slantLength,
        int lines, const Color& color) -> void;
}