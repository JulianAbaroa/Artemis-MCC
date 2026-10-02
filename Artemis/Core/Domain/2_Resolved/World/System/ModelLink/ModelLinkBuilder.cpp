module Resolved.World.System;
import :ModelLink;

import Resolved.Definitions.Type;
import Resolved.World.Type;
import std;

namespace
{
    using Resolved::World::Type::ModelLink::AnchorSource;

    // ModelTarget.LockOnFlags: Headshot = bit 0
    constexpr std::uint32_t k_LockOnHeadshot{ (1u << 0) };
}

namespace Resolved::World::System
{
    auto ModelLinkBuilder::Build(const ResolvedHlmt& hlmt, const ResolvedColl* coll,
        const ResolvedMode* mode) const -> ResolvedModelLink
    {
        ResolvedModelLink out;

        out.CollTagName = coll ? coll->TagName : std::string{};
        out.ModeTagName = mode ? mode->TagName : std::string{};

        out.CollNodeToModelNode = this->BuildNodeMap(coll, mode);
        out.SectionToRegion = this->BuildSectionToRegion(hlmt, coll);
        out.Targets = this->BuildTargets(hlmt, mode);
        out.RegionAnchors = this->BuildRegionAnchors(coll, out.CollNodeToModelNode);
        out.ObjectCenter = this->BuildObjectCenter(mode);

        return out;
    }

    auto ModelLinkBuilder::BuildNodeMap(const ResolvedColl* coll,
        const ResolvedMode* mode) const -> std::vector<std::int32_t>
    {
        std::vector<std::int32_t> map;
        if (!coll) return map;

        map.assign(coll->Nodes.size(), -1);
        if (!mode) return map;

        for (std::size_t c = 0; c < coll->Nodes.size(); ++c)
        {
            for (std::size_t m = 0; m < mode->Nodes.size(); ++m)
            {
                if (mode->Nodes[m].Name == coll->Nodes[c].Name)
                {
                    map[c] = static_cast<std::int32_t>(m);
                    break;
                }
            }
        }

        return map;
    }

    auto ModelLinkBuilder::BuildSectionToRegion(const ResolvedHlmt& hlmt,
        const ResolvedColl* coll) const -> std::vector<std::int32_t>
    {
        // Engine convention: damage section name == coll region name
        std::vector<std::int32_t> map(hlmt.DamageSections.size(), -1);
        if (!coll) return map;

        for (std::size_t s = 0; s < hlmt.DamageSections.size(); ++s)
        {
            for (std::size_t r = 0; r < coll->RegionNames.size(); ++r)
            {
                if (coll->RegionNames[r] == hlmt.DamageSections[s].Name)
                {
                    map[s] = static_cast<std::int32_t>(r);
                    break;
                }
            }
        }

        return map;
    }

    auto ModelLinkBuilder::BuildTargets(const ResolvedHlmt& hlmt,
        const ResolvedMode* mode) const -> std::vector<Anchor>
    {
        std::vector<Anchor> targets(hlmt.ModelTargets.size());
        if (!mode) return targets;

        for (std::size_t i = 0; i < hlmt.ModelTargets.size(); ++i)
        {
            const auto& target = hlmt.ModelTargets[i];
            Anchor& out = targets[i];

            // Not tied to a resolved marker, but keep what the hlmt says
            out.Radius = target.Size;
            out.Relevance = target.TargetingRelevance;
            out.ConeAngle = target.ConeAngle;
            out.SectionIndex = target.DamageSectionIndex;
            out.Headshot = (target.LockOnFlags & k_LockOnHeadshot) != 0;

            for (const auto& group : mode->MarkerGroups)
            {
                if (group.NameId != target.MarkerName) continue;
                if (group.Markers.empty()) break;

                // A group can hold several markers; the first one is used
                const auto& marker = group.Markers.front();
                if (marker.NodeIndex < 0 ||
                    static_cast<std::size_t>(marker.NodeIndex) >= mode->Nodes.size())
                {
                    break;
                }

                out.ModelNodeIndex = marker.NodeIndex;
                out.LocalOffset = marker.Translation;
                out.Source = AnchorSource::ModelTarget;
                break;
            }
        }

        return targets;
    }

    auto ModelLinkBuilder::BuildRegionAnchors(const ResolvedColl* coll,
        const std::vector<std::int32_t>& nodeMap) const -> std::vector<Anchor>
    {
        std::vector<Anchor> anchors;
        if (!coll) return anchors;

        anchors.assign(coll->RegionNames.size(), Anchor{});

        for (std::size_t r = 0; r < coll->RegionNames.size(); ++r)
        {
            // Prefer the default permutation if it has geometry, otherwise use all of them
            const int defaultPermutation = r < coll->DefaultPermutationIndex.size()
                ? coll->DefaultPermutationIndex[r] : -1;

            bool hasDefault = false;
            if (defaultPermutation >= 0)
            {
                for (const auto& mesh : coll->Meshes)
                {
                    if (mesh.RegionIndex == static_cast<std::int16_t>(r) &&
                        mesh.PermutationIndex == defaultPermutation)
                    {
                        hasDefault = true;
                        break;
                    }
                }
            }

            // Union of the mesh bounds per coll node
            struct NodeBounds
            {
                bool Used{ false };
                float MinX{}, MinY{}, MinZ{}, MaxX{}, MaxY{}, MaxZ{};
            };
            std::vector<NodeBounds> perNode(coll->Nodes.size());

            for (const auto& mesh : coll->Meshes)
            {
                if (mesh.RegionIndex != static_cast<std::int16_t>(r)) continue;
                if (hasDefault && mesh.PermutationIndex != defaultPermutation) continue;
                if (mesh.NodeIndex < 0 ||
                    static_cast<std::size_t>(mesh.NodeIndex) >= perNode.size()) continue;

                NodeBounds& b = perNode[mesh.NodeIndex];
                if (!b.Used)
                {
                    b = { true, mesh.LocalMin.X, mesh.LocalMin.Y, mesh.LocalMin.Z,
                                mesh.LocalMax.X, mesh.LocalMax.Y, mesh.LocalMax.Z };
                    continue;
                }
                b.MinX = (std::min)(b.MinX, mesh.LocalMin.X);
                b.MinY = (std::min)(b.MinY, mesh.LocalMin.Y);
                b.MinZ = (std::min)(b.MinZ, mesh.LocalMin.Z);
                b.MaxX = (std::max)(b.MaxX, mesh.LocalMax.X);
                b.MaxY = (std::max)(b.MaxY, mesh.LocalMax.Y);
                b.MaxZ = (std::max)(b.MaxZ, mesh.LocalMax.Z);
            }

            // The region is anchored to the node holding the largest volume of it
            int bestNode = -1;
            float bestVolume = -1.0f;
            for (std::size_t n = 0; n < perNode.size(); ++n)
            {
                const NodeBounds& b = perNode[n];
                if (!b.Used) continue;
                if (n >= nodeMap.size() || nodeMap[n] < 0) continue;

                const float volume = (b.MaxX - b.MinX) * (b.MaxY - b.MinY) * (b.MaxZ - b.MinZ);
                if (volume > bestVolume)
                {
                    bestVolume = volume;
                    bestNode = static_cast<int>(n);
                }
            }
            if (bestNode < 0) continue;

            const NodeBounds& b = perNode[bestNode];
            Anchor& out = anchors[r];
            out.ModelNodeIndex = nodeMap[bestNode];
            out.LocalOffset.X = (b.MinX + b.MaxX) * 0.5f;
            out.LocalOffset.Y = (b.MinY + b.MaxY) * 0.5f;
            out.LocalOffset.Z = (b.MinZ + b.MaxZ) * 0.5f;
            out.Radius = (std::max)({ b.MaxX - b.MinX, b.MaxY - b.MinY, b.MaxZ - b.MinZ }) * 0.5f;
            out.Source = AnchorSource::CollRegion;
        }

        return anchors;
    }

    auto ModelLinkBuilder::BuildObjectCenter(const ResolvedMode* mode) const -> Anchor
    {
        Anchor out;
        if (!mode) return out;

        const auto& b = mode->ModelBounds;
        const float sx = b.Max.X - b.Min.X;
        const float sy = b.Max.Y - b.Min.Y;
        const float sz = b.Max.Z - b.Min.Z;
        if (sx <= 0.0f && sy <= 0.0f && sz <= 0.0f) return out;

        out.ModelNodeIndex = -1;
        out.LocalOffset.X = (b.Min.X + b.Max.X) * 0.5f;
        out.LocalOffset.Y = (b.Min.Y + b.Max.Y) * 0.5f;
        out.LocalOffset.Z = (b.Min.Z + b.Max.Z) * 0.5f;
        out.Radius = (std::max)({ sx, sy, sz }) * 0.5f;
        out.Source = AnchorSource::ObjectCenter;
        return out;
    }
}