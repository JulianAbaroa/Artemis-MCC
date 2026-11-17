module DevTools.Viewer.Geometry.System;

import Common.Math.Type;
import Common.Math.System;
import std;

namespace
{
    namespace VecMath = Common::Math::System;

    using Engine::Render::Type::Vertex;
    using Common::Math::Type::Vec3;
    using DevTools::Viewer::Style::Type::Color;
    using Common::ZoneShape::Type::ZoneShape;
    using Common::ZoneShape::Type::ZoneShapeKind;

    using Common::Math::Type::k_TwoPi;

    constexpr int k_CylinderSegments{ 24 };
    constexpr float k_DashLength{ 0.6f };
    constexpr float k_DashGap{ 0.4f };
    constexpr float k_HeadLength{ 0.45f };
    constexpr float k_HeadWidth{ 0.2f };
    constexpr float k_Minimum{ 1e-5f };

    struct Basis
    {
        Vec3 Right{};
        Vec3 Forward{};
        Vec3 Up{};
    };

    // Builds an orthonormal basis. The up direction is corrected so it is perpendicular to forward.
    auto MakeBasis(const Vec3& forward, const Vec3& up) -> Basis
    {
        Basis basis{};
        basis.Forward = VecMath::Normalize(forward);
        basis.Up = VecMath::Normalize(up);
        basis.Right = VecMath::Normalize(VecMath::Cross(basis.Forward, basis.Up));
        basis.Up = VecMath::Normalize(VecMath::Cross(basis.Right, basis.Forward));
        return basis;
    }

    // Converts offsets along the basis axes, measured from the center, to a world position.
    auto ToWorld(const Vec3& center, const Basis& basis,
        float right, float forward, float up) -> Vec3
    {
        return Vec3{
            center.X + basis.Right.X * right + basis.Forward.X * forward + basis.Up.X * up,
            center.Y + basis.Right.Y * right + basis.Forward.Y * forward + basis.Up.Y * up,
            center.Z + basis.Right.Z * right + basis.Forward.Z * forward + basis.Up.Z * up };
    }

    auto MakeVertex(const Vec3& position, const Color& color) -> Vertex
    {
        return Vertex{ position.X, position.Y, position.Z, color.R, color.G, color.B };
    }

    // return: A basis whose Right and Up are perpendicular to the axis, whatever its direction.
    auto BasisAround(const Vec3& axis) -> Basis
    {
        const bool isVertical{ std::fabs(VecMath::Normalize(axis).Z) > 0.99f };
        return MakeBasis(axis, isVertical ? Vec3{ 1.0f, 0.0f, 0.0f } : Vec3{ 0.0f, 0.0f, 1.0f });
    }

    auto AppendHead(std::vector<Vertex>& out, const Vec3& tip, const Vec3& direction, const Color& color) -> void
    {
        const Basis basis{ BasisAround(direction) };
        const Vec3 baseCenter{ VecMath::Subtract(tip, VecMath::Scale(direction, k_HeadLength)) };

        for (const Vec3& side : { basis.Right, basis.Up })
        {
            const Vec3 offset{ VecMath::Scale(side, k_HeadWidth) };

            out.push_back(MakeVertex(tip, color));
            out.push_back(MakeVertex(VecMath::Add(baseCenter, offset), color));
            out.push_back(MakeVertex(tip, color));
            out.push_back(MakeVertex(VecMath::Subtract(baseCenter, offset), color));
        }
    }
}

namespace DevTools::Viewer::Geometry::System
{
    auto AppendLine(std::vector<Vertex>& out, const Vec3& from, const Vec3& to, const Color& color) -> void
    {
        out.push_back(MakeVertex(from, color));
        out.push_back(MakeVertex(to, color));
    }

    auto AppendArrow(std::vector<Vertex>& out, const Vec3& from, const Vec3& to, const Color& color, bool dashed) -> void
    {
        const Vec3 delta{ VecMath::Subtract(to, from) };
        const float length{ VecMath::Length(delta) };
        if (length < k_Minimum) return;

        const Vec3 direction{ VecMath::Scale(delta, 1.0f / length) };

        if (!dashed)
        {
            AppendLine(out, from, to, color);
        }
        else
        {
            for (float at{ 0.0f }; at < length; at += k_DashLength + k_DashGap)
            {
                const float end{ (std::min)(at + k_DashLength, length) };
                AppendLine(out, VecMath::Add(from, VecMath::Scale(direction, at)),
                    VecMath::Add(from, VecMath::Scale(direction, end)), color);
            }
        }

        AppendHead(out, to, direction, color);
    }

    auto AppendCircle(std::vector<Vertex>& out, const Vec3& center, const Vec3& normal, float radius,
        int segments, const Color& color) -> void
    {
        if (segments < 3) return;

        const Basis basis{ BasisAround(normal) };
        const auto pointAt = [&](int index) -> Vec3
        {
            const float angle{ k_TwoPi * static_cast<float>(index) / static_cast<float>(segments) };
            return ToWorld(center, basis, radius * std::cos(angle), 0.0f, radius * std::sin(angle));
        };

        for (int index{ 0 }; index < segments; ++index)
        {
            AppendLine(out, pointAt(index), pointAt(index + 1), color);
        }
    }

    auto AppendCross(std::vector<Vertex>& out, const Vec3& center, float halfSize, const Color& color) -> void
    {
        AppendLine(out, Vec3{ center.X - halfSize, center.Y, center.Z }, Vec3{ center.X + halfSize, center.Y, center.Z }, color);
        AppendLine(out, Vec3{ center.X, center.Y - halfSize, center.Z }, Vec3{ center.X, center.Y + halfSize, center.Z }, color);
        AppendLine(out, Vec3{ center.X, center.Y, center.Z - halfSize }, Vec3{ center.X, center.Y, center.Z + halfSize }, color);
    }

    auto AppendCone(std::vector<Vertex>& out, const Vec3& apex, const Vec3& axis, float halfAngle, float slantLength,
        int lines, const Color& color) -> void
    {
        if (lines < 1) return;

        const Basis basis{ BasisAround(axis) };
        const float along{ slantLength * std::cos(halfAngle) };
        const float radius{ slantLength * std::sin(halfAngle) };
        const Vec3 center{ ToWorld(apex, basis, 0.0f, along, 0.0f) };

        AppendCircle(out, center, axis, radius, k_CylinderSegments, color);

        for (int index{ 0 }; index < lines; ++index)
        {
            const float angle{ k_TwoPi * static_cast<float>(index) / static_cast<float>(lines) };
            AppendLine(out, apex, ToWorld(apex, basis, radius * std::cos(angle), along, radius * std::sin(angle)), color);
        }
    }

    auto AppendSolid(std::vector<Vertex>& out, const Vec3& position,
        const Vec3& forward, const Vec3& up, const ZoneShape& zone,
        const Color& color) -> void
    {
        const Basis basis = MakeBasis(forward, up);

        const float top = zone.Top;
        const float bottom = -zone.Bottom;

        auto world = [&](float right, float forwardOffset, float upOffset)
        {
            return ToWorld(position, basis, right, forwardOffset, upOffset);
        };

        auto push = [&](const Vec3& point)
        {
            out.push_back(MakeVertex(point, color));
        };

        if (zone.Kind == ZoneShapeKind::Cylinder)
        {
            const float radius = zone.Radius;

            for (int i = 0; i < k_CylinderSegments; ++i)
            {
                const float angle0 = (static_cast<float>(i) / k_CylinderSegments) * k_TwoPi;
                const float angle1 = (static_cast<float>(i + 1) / k_CylinderSegments) * k_TwoPi;

                const float x0 = std::cos(angle0) * radius, y0 = std::sin(angle0) * radius;
                const float x1 = std::cos(angle1) * radius, y1 = std::sin(angle1) * radius;

                const Vec3 bottomLeft = world(x0, y0, bottom);
                const Vec3 bottomRight = world(x1, y1, bottom);
                const Vec3 topLeft = world(x0, y0, top);
                const Vec3 topRight = world(x1, y1, top);

                push(bottomLeft); push(bottomRight); push(topRight);
                push(bottomLeft); push(topRight); push(topLeft);

                push(world(0.0f, 0.0f, top)); push(topLeft); push(topRight);

                push(world(0.0f, 0.0f, bottom)); push(bottomRight); push(bottomLeft);
            }
        }
        else if (zone.Kind == ZoneShapeKind::Box)
        {
            const float halfWidth = zone.Radius * 0.5f;
            const float halfLength = zone.Length * 0.5f;

            const std::array<Vec3, 8> corner = {
                world(-halfWidth, -halfLength, bottom), world(halfWidth, -halfLength, bottom),
                world(halfWidth, halfLength, bottom), world(-halfWidth, halfLength, bottom),
                world(-halfWidth, -halfLength, top), world(halfWidth, -halfLength, top),
                world(halfWidth, halfLength, top), world(-halfWidth, halfLength, top)
            };

            auto quad = [&](int a, int b, int c, int d)
            {
                push(corner[a]); push(corner[b]); push(corner[c]);
                push(corner[a]); push(corner[c]); push(corner[d]);
            };

            quad(0, 1, 2, 3);
            quad(4, 5, 6, 7);
            quad(0, 1, 5, 4);
            quad(2, 3, 7, 6);
            quad(1, 2, 6, 5);
            quad(3, 0, 4, 7);
        }
    }
}