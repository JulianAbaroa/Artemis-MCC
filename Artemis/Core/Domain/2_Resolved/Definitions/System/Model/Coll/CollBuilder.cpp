module Resolved.Definitions.System;
import :Coll;

import Common.Math.Type;
import Resolved.Definitions.Type;

namespace
{
    using Common::Math::Type::Triangle;
    using Resolved::Definitions::Type::Coll::CollMesh;
    using Resolved::Definitions::Type::Coll::Material;

    using Common::Math::Type::k_FloatMax;
    using Resolved::Definitions::Type::Coll::k_DefaultPermutationName;
}

namespace Resolved::Definitions::System
{
    auto CollBuilder::Build(const CollObject& coll) -> Coll
    {
        Coll out{};

        out.TagName = coll.TagName;

        this->BuildNodes(coll, out);
        this->BuildMeshes(coll, out);
        this->BuildMaterials(coll, out);
        this->BuildBounds(out);

        return out;
    }

    auto CollBuilder::BuildNodes(const CollObject& coll, Coll& out) -> void
    {
        out.Nodes.reserve(coll.Nodes.size());

        for (const auto& src : coll.Nodes)
        {
            Node node{};

            node.Name = src.Name;
            node.ParentIndex = src.ParentNodeIndex;
            node.NextSiblingIndex = src.NextSiblingNodeIndex;
            node.FirstChildIndex = src.FirstChildNodeIndex;

            out.Nodes.push_back(node);
        }
    }

    auto CollBuilder::BuildMeshes(const CollObject& coll, Coll& out) -> void
    {
        for (std::int16_t i = 0;
            i < static_cast<std::int16_t>(coll.Regions.size());
            ++i)
        {
            const auto& region = coll.Regions[i];

            for (std::int16_t j = 0;
                j < static_cast<std::int16_t>(region.Permutations.size());
                ++j)
            {
                const auto& permutation = region.Permutations[j];

                for (const auto& bsp : permutation.Bsps)
                {
                    CollMesh mesh{};

                    mesh.NodeIndex = bsp.NodeIndex;
                    mesh.RegionIndex = i;
                    mesh.PermutationIndex = j;

                    mesh.Triangles.reserve(bsp.Surfaces.size());

                    for (std::int32_t k = 0;
                        k < static_cast<std::int32_t>(bsp.Surfaces.size());
                        ++k)
                    {
                        const std::vector<std::int32_t> indices =
                            this->CollectSurfaceVertexIndices(bsp, k);

                        if (indices.size() < 3) continue;

                        const auto& surface = bsp.Surfaces[k];

                        for (std::size_t fan = 1; fan + 1 < indices.size(); ++fan)
                        {
                            Triangle triangle{};

                            triangle.A = bsp.Vertices[indices[0]].Point;

                            triangle.B = bsp.Vertices[indices[fan]].Point;

                            triangle.C = bsp.Vertices[indices[fan + 1]].Point;

                            triangle.SurfaceFlags = surface.Flags;
                            triangle.Material = surface.Material;
                            mesh.Triangles.push_back(triangle);
                        }
                    }

                    if (mesh.Triangles.empty()) continue;

                    mesh.LocalMin = { k_FloatMax, k_FloatMax, k_FloatMax };
                    mesh.LocalMax = { -k_FloatMax, -k_FloatMax, -k_FloatMax };

                    auto expandBounds = [&](const Vec3& q)
                    {
                        mesh.LocalMin.X = (std::min)(mesh.LocalMin.X, q.X);
                        mesh.LocalMin.Y = (std::min)(mesh.LocalMin.Y, q.Y);
                        mesh.LocalMin.Z = (std::min)(mesh.LocalMin.Z, q.Z);
                        mesh.LocalMax.X = (std::max)(mesh.LocalMax.X, q.X);
                        mesh.LocalMax.Y = (std::max)(mesh.LocalMax.Y, q.Y);
                        mesh.LocalMax.Z = (std::max)(mesh.LocalMax.Z, q.Z);
                    };

                    for (const auto& t : mesh.Triangles)
                    {
                        expandBounds(t.A);
                        expandBounds(t.B);
                        expandBounds(t.C);
                    }

                    out.Meshes.push_back(std::move(mesh));
                }
            }
        }

        out.RegionNames.assign(coll.Regions.size(), 0);
        out.PermutationNames.assign(coll.Regions.size(), {});
        out.DefaultPermutationIndex.assign(coll.Regions.size(), -1);

        for (std::size_t region = 0; region < coll.Regions.size(); ++region)
        {
            out.RegionNames[region] = coll.Regions[region].Name;

            const auto& permutations = coll.Regions[region].Permutations;
            out.PermutationNames[region].resize(permutations.size());

            for (std::size_t i = 0; i < permutations.size(); ++i)
            {
                out.PermutationNames[region][i] = permutations[i].Name;

                if (permutations[i].Name == k_DefaultPermutationName)
                {
                    out.DefaultPermutationIndex[region] = static_cast<int>(i);
                }
            }
        }
    }

    auto CollBuilder::BuildMaterials(const CollObject& coll, Coll& out) -> void
    {
        out.Materials.reserve(coll.Materials.size());

        for (const auto& material : coll.Materials)
        {
            Material resolved{};

            resolved.Name = material.Name;

            out.Materials.push_back(resolved);
        }
    }

    auto CollBuilder::BuildBounds(Coll& out) -> void
    {
        if (out.Meshes.empty())
        {
            out.BoundsMin = {};
            out.BoundsMax = {};
            return;
        }

        out.BoundsMin = { k_FloatMax, k_FloatMax, k_FloatMax };
        out.BoundsMax = { -k_FloatMax, -k_FloatMax, -k_FloatMax };

        for (const auto& mesh : out.Meshes)
        {
            out.BoundsMin.X = (std::min)(out.BoundsMin.X, mesh.LocalMin.X);
            out.BoundsMin.Y = (std::min)(out.BoundsMin.Y, mesh.LocalMin.Y);
            out.BoundsMin.Z = (std::min)(out.BoundsMin.Z, mesh.LocalMin.Z);
            out.BoundsMax.X = (std::max)(out.BoundsMax.X, mesh.LocalMax.X);
            out.BoundsMax.Y = (std::max)(out.BoundsMax.Y, mesh.LocalMax.Y);
            out.BoundsMax.Z = (std::max)(out.BoundsMax.Z, mesh.LocalMax.Z);
        }
    }

    auto CollBuilder::CollectSurfaceVertexIndices(
        const Coll_Regions_Permutations_BspsObject& bsp,
        std::int32_t surfaceIndex) -> std::vector<std::int32_t>
    {
        std::vector<std::int32_t> ring{};

        const auto& surfaces = bsp.Surfaces;
        const auto& edges = bsp.Edges;

        if (surfaceIndex < 0 ||
            static_cast<std::size_t>(surfaceIndex) >= surfaces.size())
        {
            return ring;
        }

        const std::int32_t firstEdge = surfaces[surfaceIndex].FirstEdge;

        if (firstEdge < 0 ||
            static_cast<std::size_t>(firstEdge) >= edges.size())
        {
            return ring;
        }

        std::int32_t i{ firstEdge };

        const std::size_t maxSteps = edges.size() + 1;
        std::size_t steps{};

        do
        {
            if (i < 0 || static_cast<std::size_t>(i) >= edges.size())
            {
                ring.clear();
                return ring;
            }

            const auto& edge = edges[i];

            std::int32_t vertex{};
            std::int32_t next{};

            if (edge.LeftSurface == surfaceIndex)
            {
                vertex = edge.StartVertex;
                next = edge.ForwardEdge;
            }
            else
            {
                vertex = edge.EndVertex;
                next = edge.ReverseEdge;
            }

            if (vertex < 0 ||
                static_cast<std::size_t>(vertex) >= bsp.Vertices.size())
            {
                ring.clear();
                return ring;
            }

            ring.push_back(vertex);
            i = next;

            if (++steps > maxSteps)
            {
                ring.clear();
                return ring;
            }
        } while (i != firstEdge);

        return ring;
    }
}