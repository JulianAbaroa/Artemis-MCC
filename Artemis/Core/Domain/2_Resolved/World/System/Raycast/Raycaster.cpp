module;

#include <cmath>

module Resolved.World.System;
import :Raycast;

import Common.Geometry.System;
import Common.Math.Type;
import Resolved.World.Type;

namespace
{
    constexpr float k_Max{ (std::numeric_limits<float>::max)() };

    // Triangles a leaf holds before it is split.
    constexpr int k_LeafSize{ 4 };

    // Deepest level of the hierarchy, so the traversal stack stays bounded.
    constexpr int k_MaxDepth{ 60 };

    // Slots of the traversal stack.
    constexpr int k_StackSize{ 128 };

    // Bins used to choose where a node splits.
    constexpr int k_Bins{ 16 };

    // Padding of every box, so rays that graze a triangle edge are not lost to rounding.
    constexpr float k_BoxPadding{ 1.0e-3f };

    // Inverse of a direction component, finite when the component is zero.
    auto SafeInverse(float value) -> float
    {
        if (std::fabs(value) < 1.0e-20f) return 1.0e20f;
        return 1.0f / value;
    }

    // Triangle with the data the builder needs.
    struct Prim
    {
        Common::Math::Type::Triangle Tri{};
        std::array<float, 3> Min{};
        std::array<float, 3> Max{};
        std::array<float, 3> Center{};
    };

    struct Bounds
    {
        std::array<float, 3> Min{ k_Max, k_Max, k_Max };
        std::array<float, 3> Max{ -k_Max, -k_Max, -k_Max };

        auto Include(const std::array<float, 3>& point) -> void
        {
            for (int axis = 0; axis < 3; ++axis)
            {
                Min[axis] = (std::min)(Min[axis], point[axis]);
                Max[axis] = (std::max)(Max[axis], point[axis]);
            }
        }

        auto Include(const Bounds& other) -> void
        {
            this->Include(other.Min);
            this->Include(other.Max);
        }

        auto Area() const -> float
        {
            const float dx = Max[0] - Min[0];
            const float dy = Max[1] - Min[1];
            const float dz = Max[2] - Min[2];
            if (dx < 0.0f) return 0.0f;
            return 2.0f * (dx * dy + dy * dz + dz * dx);
        }
    };

    // Fills a node and, when it has many triangles, its two children.
    // note: Splits at the best bin of a surface area heuristic over the longest axis of the centers.
    auto FillNode(std::vector<Prim>& prims, std::size_t begin, std::size_t end, std::int32_t nodeIndex,
        std::vector<Resolved::World::Type::Raycast::BvhNode>& nodes, int depth) -> void
    {
        Bounds bounds{};
        Bounds centers{};
        for (std::size_t i = begin; i < end; ++i)
        {
            bounds.Include(prims[i].Min);
            bounds.Include(prims[i].Max);
            centers.Include(prims[i].Center);
        }

        for (int axis = 0; axis < 3; ++axis)
        {
            nodes[static_cast<std::size_t>(nodeIndex)].Min[axis] = bounds.Min[axis] - k_BoxPadding;
            nodes[static_cast<std::size_t>(nodeIndex)].Max[axis] = bounds.Max[axis] + k_BoxPadding;
        }

        const std::size_t count{ end - begin };

        int axis{ 0 };
        float extent{ centers.Max[0] - centers.Min[0] };
        for (int a = 1; a < 3; ++a)
        {
            const float candidate = centers.Max[a] - centers.Min[a];
            if (candidate > extent)
            {
                extent = candidate;
                axis = a;
            }
        }

        if (count <= static_cast<std::size_t>(k_LeafSize) || depth >= k_MaxDepth || extent <= 0.0f)
        {
            nodes[static_cast<std::size_t>(nodeIndex)].Left = static_cast<std::int32_t>(begin);
            nodes[static_cast<std::size_t>(nodeIndex)].Count = static_cast<std::int32_t>(count);
            return;
        }

        auto binOf = [&](const Prim& prim) -> int {
            const int bin = static_cast<int>(static_cast<float>(k_Bins) * (prim.Center[axis] - centers.Min[axis]) / extent);
            return (std::clamp)(bin, 0, k_Bins - 1);
        };

        std::array<Bounds, k_Bins> binBounds{};
        std::array<int, k_Bins> binCounts{};
        for (std::size_t i = begin; i < end; ++i)
        {
            const int bin = binOf(prims[i]);
            binBounds[bin].Include(prims[i].Min);
            binBounds[bin].Include(prims[i].Max);
            ++binCounts[bin];
        }

        // Cost of keeping bins 0..i on the left, and bins i..last on the right.
        std::array<float, k_Bins> leftCost{};
        std::array<float, k_Bins> rightCost{};
        {
            Bounds running{};
            int total{};
            for (int i = 0; i < k_Bins; ++i)
            {
                running.Include(binBounds[i]);
                total += binCounts[i];
                leftCost[i] = total > 0 ? running.Area() * static_cast<float>(total) : 0.0f;
            }
        }
        {
            Bounds running{};
            int total{};
            for (int i = k_Bins - 1; i >= 0; --i)
            {
                running.Include(binBounds[i]);
                total += binCounts[i];
                rightCost[i] = total > 0 ? running.Area() * static_cast<float>(total) : 0.0f;
            }
        }

        int bestSplit{ 0 };
        float bestCost{ k_Max };
        for (int i = 0; i < k_Bins - 1; ++i)
        {
            const float cost = leftCost[i] + rightCost[i + 1];
            if (cost < bestCost)
            {
                bestCost = cost;
                bestSplit = i;
            }
        }

        auto first = prims.begin() + static_cast<std::ptrdiff_t>(begin);
        auto last = prims.begin() + static_cast<std::ptrdiff_t>(end);
        auto middle = std::partition(first, last, [&](const Prim& prim) {
            return binOf(prim) <= bestSplit;
        });

        // All triangles fell on one side, so the node splits in half.
        if (middle == first || middle == last)
        {
            middle = first + static_cast<std::ptrdiff_t>(count / 2);
            std::nth_element(first, middle, last, [&](const Prim& a, const Prim& b) {
                return a.Center[axis] < b.Center[axis];
            });
        }

        const std::size_t mid = static_cast<std::size_t>(middle - prims.begin());

        const std::int32_t left = static_cast<std::int32_t>(nodes.size());
        nodes.resize(nodes.size() + 2);
        nodes[static_cast<std::size_t>(nodeIndex)].Left = left;
        nodes[static_cast<std::size_t>(nodeIndex)].Count = 0;

        FillNode(prims, begin, mid, left, nodes, depth + 1);
        FillNode(prims, mid, end, left + 1, nodes, depth + 1);
    }

    // Distance at which a ray enters a box, or false when it misses it before tMax.
    auto EntersBox(const Resolved::World::Type::Raycast::BvhNode& node, const std::array<float, 3>& origin,
        const std::array<float, 3>& inverse, float tMax, float& outEnter) -> bool
    {
        float t0{ 0.0f };
        float t1{ tMax };
        for (int axis = 0; axis < 3; ++axis)
        {
            float ta = (node.Min[axis] - origin[axis]) * inverse[axis];
            float tb = (node.Max[axis] - origin[axis]) * inverse[axis];
            if (ta > tb) std::swap(ta, tb);
            t0 = (std::max)(t0, ta);
            t1 = (std::min)(t1, tb);
            if (t0 > t1) return false;
        }

        outEnter = t0;
        return true;
    }
}

namespace Resolved::World::System
{
    auto Raycaster::Build(const DefinitionsStore& definitionsStore) -> void
    {
        m_Nodes.clear();
        m_Triangles.clear();

        std::vector<const Sbsp*> sbsps{};

        // The store is unordered, so the list is sorted for a deterministic hierarchy.
        for (const auto& [tagName, sbsp] : definitionsStore.Sbsp.All())
        {
            sbsps.push_back(&sbsp);
        }

        std::ranges::sort(sbsps, [](const Sbsp* a, const Sbsp* b) {
            return a->TagName < b->TagName;
        });

        this->BuildBvh(sbsps);
    }

    auto Raycaster::BuildBvh(const std::vector<const Sbsp*>& sbsps) -> void
    {

        std::vector<Prim> prims{};
        for (const Sbsp* sbsp : sbsps)
        {
            for (const auto& tri : sbsp->RenderGeometry)
            {
                Prim prim{};
                prim.Tri = tri;
                prim.Min = {
                    (std::min)({ tri.A.X, tri.B.X, tri.C.X }),
                    (std::min)({ tri.A.Y, tri.B.Y, tri.C.Y }),
                    (std::min)({ tri.A.Z, tri.B.Z, tri.C.Z })
                };
                prim.Max = {
                    (std::max)({ tri.A.X, tri.B.X, tri.C.X }),
                    (std::max)({ tri.A.Y, tri.B.Y, tri.C.Y }),
                    (std::max)({ tri.A.Z, tri.B.Z, tri.C.Z })
                };
                prim.Center = {
                    (prim.Min[0] + prim.Max[0]) * 0.5f,
                    (prim.Min[1] + prim.Max[1]) * 0.5f,
                    (prim.Min[2] + prim.Max[2]) * 0.5f
                };
                prims.push_back(prim);
            }
        }

        if (prims.empty()) return;

        m_Nodes.resize(1);
        FillNode(prims, 0, prims.size(), 0, m_Nodes, 0);

        m_Triangles.reserve(prims.size());
        for (const Prim& prim : prims)
        {
            m_Triangles.push_back(prim.Tri);
        }

    }

    auto Raycaster::Cast(const Vec3& origin, const Vec3& direction, float maxDistance) const -> Hit
    {
        Hit best{};
        if (maxDistance <= 0.0f || m_Nodes.empty()) return best;

        const std::array<float, 3> rayOrigin{ origin.X, origin.Y, origin.Z };
        const std::array<float, 3> inverse{ SafeInverse(direction.X), SafeInverse(direction.Y), SafeInverse(direction.Z) };

        float closest{ maxDistance };

        std::array<std::int32_t, k_StackSize> stack{};
        int top{ 0 };
        stack[top++] = 0;

        while (top > 0)
        {
            const BvhNode& node = m_Nodes[static_cast<std::size_t>(stack[--top])];

            if (node.Count > 0)
            {
                for (std::int32_t i = node.Left; i < node.Left + node.Count; ++i)
                {
                    float distance{};
                    if (!Common::Geometry::System::RayIntersectsTriangle(
                        origin, direction, m_Triangles[static_cast<std::size_t>(i)], closest, distance))
                    {
                        continue;
                    }

                    if (!best.IsHit || distance < best.Distance)
                    {
                        best.IsHit = true;
                        best.Distance = distance;
                        closest = distance;
                    }
                }

                continue;
            }

            const std::int32_t leftIndex{ node.Left };
            float leftEnter{};
            float rightEnter{};
            const bool hitLeft = EntersBox(m_Nodes[static_cast<std::size_t>(leftIndex)], rayOrigin, inverse, closest, leftEnter);
            const bool hitRight = EntersBox(m_Nodes[static_cast<std::size_t>(leftIndex) + 1], rayOrigin, inverse, closest, rightEnter);

            // The nearer child is pushed last, so it is visited first.
            if (hitLeft && hitRight)
            {
                if (leftEnter <= rightEnter)
                {
                    stack[top++] = leftIndex + 1;
                    stack[top++] = leftIndex;
                }
                else
                {
                    stack[top++] = leftIndex;
                    stack[top++] = leftIndex + 1;
                }
            }
            else if (hitLeft)
            {
                stack[top++] = leftIndex;
            }
            else if (hitRight)
            {
                stack[top++] = leftIndex + 1;
            }
        }

        if (best.IsHit)
        {
            best.Point = {
                origin.X + direction.X * best.Distance,
                origin.Y + direction.Y * best.Distance,
                origin.Z + direction.Z * best.Distance
            };
        }

        return best;
    }

    auto Raycaster::Cleanup() -> void
    {
        m_Nodes.clear();
        m_Triangles.clear();
    }
}