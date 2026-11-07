export module Common.Geometry.System;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Triangle;
}

export namespace Common::Geometry::System
{
    // Tests a ray against a triangle (Moller-Trumbore).
    // param maxDistance: Hits farther than this are ignored.
    // param outDistance: Distance along the ray to the hit. Only written on a hit.
    // return: False if the ray misses, is parallel to the triangle or the hit is out of range.
    auto RayIntersectsTriangle(const Vec3& origin, const Vec3& direction,
        const Triangle& triangle, float maxDistance, float& outDistance) -> bool;

    // Appends the triangles of one surface of a collision bsp, with its flags and material.
    // The polygon of a surface is the ring of edges that starts at its first edge, walked on the side that faces the surface, and
    // is cut into a fan from its first vertex.
    // param surfaces: Items with FirstEdge, Flags and Material.
    // param edges: Items with StartVertex, EndVertex, ForwardEdge, ReverseEdge and LeftSurface.
    // param vertices: Items with Point.
    // return: Triangles appended. 0 if an index leaves its array, the ring does not close or it has fewer than 3 vertices.
    template <typename Surfaces, typename Edges, typename Vertices>
    auto AppendCollisionSurface(const Surfaces& surfaces, const Edges& edges, const Vertices& vertices,
        std::int32_t surfaceIndex, std::vector<Triangle>& out) -> std::uint32_t
    {
        if (surfaceIndex < 0 || static_cast<std::size_t>(surfaceIndex) >= surfaces.size()) return 0;

        const auto& surface = surfaces[surfaceIndex];
        const std::int32_t firstEdge{ static_cast<std::int32_t>(surface.FirstEdge) };

        if (firstEdge < 0 || static_cast<std::size_t>(firstEdge) >= edges.size()) return 0;

        std::vector<std::int32_t> ring{};
        std::int32_t i{ firstEdge };

        const std::size_t maxSteps{ edges.size() + 1 };
        std::size_t steps{};

        do
        {
            if (i < 0 || static_cast<std::size_t>(i) >= edges.size()) return 0;

            const auto& edge = edges[i];

            std::int32_t vertex{};
            std::int32_t next{};

            if (static_cast<std::int32_t>(edge.LeftSurface) == surfaceIndex)
            {
                vertex = edge.StartVertex;
                next = edge.ForwardEdge;
            }
            else
            {
                vertex = edge.EndVertex;
                next = edge.ReverseEdge;
            }

            if (vertex < 0 || static_cast<std::size_t>(vertex) >= vertices.size()) return 0;

            ring.push_back(vertex);
            i = next;

            if (++steps > maxSteps) return 0;
        } while (i != firstEdge);

        if (ring.size() < 3) return 0;

        std::uint32_t appended{};

        for (std::size_t fan = 1; fan + 1 < ring.size(); ++fan)
        {
            Triangle triangle{};

            triangle.A = vertices[ring[0]].Point;
            triangle.B = vertices[ring[fan]].Point;
            triangle.C = vertices[ring[fan + 1]].Point;
            triangle.SurfaceFlags = surface.Flags;
            triangle.Material = surface.Material;

            out.push_back(triangle);
            ++appended;
        }

        return appended;
    }
}