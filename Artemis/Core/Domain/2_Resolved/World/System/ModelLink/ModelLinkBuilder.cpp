module Resolved.World.System;
import :ModelLink;

namespace
{
    using Resolved::World::Type::ModelLink::AnchorSource;

    // Bit 0 of the lock-on flags of a model target: headshot.
    constexpr std::uint32_t k_LockOnHeadshot{ 1u << 0 };

    // Bounds of the meshes of one node.
    struct NodeBounds
    {
        bool IsUsed{};
        float MinX{}, MinY{}, MinZ{}, MaxX{}, MaxY{}, MaxZ{};
    };
}

namespace Resolved::World::System
{
    auto ModelLinkBuilder::Build(const Hlmt& hlmt, const Coll* coll, const Mode* mode) const -> ModelLink
    {
        ModelLink out{};

        out.CollTagName = coll ? coll->TagName : std::string{};
        out.ModeTagName = mode ? mode->TagName : std::string{};

        out.CollNodeToModelNode = this->BuildNodeMap(coll, mode);
        out.SectionToRegion = this->BuildSectionToRegion(hlmt, coll);
        out.Targets = this->BuildTargets(hlmt, mode);
        out.RegionAnchors = this->BuildRegionAnchors(coll, out.CollNodeToModelNode);
        out.ObjectCenter = this->BuildObjectCenter(mode);

        return out;
    }

    auto ModelLinkBuilder::BuildNodeMap(const Coll* coll, const Mode* mode) const -> std::vector<std::int32_t>
    {
        std::vector<std::int32_t> nodeMap{};
        if (!coll) return nodeMap;

        nodeMap.assign(coll->Nodes.size(), -1);
        if (!mode) return nodeMap;

        for (std::size_t collNode = 0; collNode < coll->Nodes.size(); ++collNode)
        {
            for (std::size_t modeNode = 0; modeNode < mode->Nodes.size(); ++modeNode)
            {
                if (mode->Nodes[modeNode].Name == coll->Nodes[collNode].Name)
                {
                    nodeMap[collNode] = static_cast<std::int32_t>(modeNode);
                    break;
                }
            }
        }

        return nodeMap;
    }

    auto ModelLinkBuilder::BuildSectionToRegion(const Hlmt& hlmt, const Coll* coll) const -> std::vector<std::int32_t>
    {
        std::vector<std::int32_t> sectionToRegion(hlmt.DamageSections.size(), -1);
        if (!coll) return sectionToRegion;

        for (std::size_t section = 0; section < hlmt.DamageSections.size(); ++section)
        {
            for (std::size_t region = 0; region < coll->RegionNames.size(); ++region)
            {
                if (coll->RegionNames[region] == hlmt.DamageSections[section].Name)
                {
                    sectionToRegion[section] = static_cast<std::int32_t>(region);
                    break;
                }
            }
        }

        return sectionToRegion;
    }

    auto ModelLinkBuilder::BuildTargets(const Hlmt& hlmt, const Mode* mode) const -> std::vector<Anchor>
    {
        std::vector<Anchor> targets(hlmt.ModelTargets.size());
        if (!mode) return targets;

        for (std::size_t i = 0; i < hlmt.ModelTargets.size(); ++i)
        {
            const auto& target = hlmt.ModelTargets[i];
            Anchor& out = targets[i];

            out.Radius = target.Size;
            out.Relevance = target.TargetingRelevance;
            out.ConeAngle = target.ConeAngle;
            out.SectionIndex = target.DamageSectionIndex;
            out.Headshot = (target.LockOnFlags & k_LockOnHeadshot) != 0;

            for (const auto& group : mode->MarkerGroups)
            {
                if (group.NameId != target.MarkerName) continue;
                if (group.Markers.empty()) break;

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

    auto ModelLinkBuilder::BuildRegionAnchors(const Coll* coll,
        const std::vector<std::int32_t>& nodeMap) const -> std::vector<Anchor>
    {
        std::vector<Anchor> anchors{};
        if (!coll) return anchors;

        anchors.assign(coll->RegionNames.size(), Anchor{});

        for (std::size_t region = 0; region < coll->RegionNames.size(); ++region)
        {
            const int defaultPermutation = region < coll->DefaultPermutationIndex.size()
                ? coll->DefaultPermutationIndex[region] : -1;

            bool hasDefault{};
            if (defaultPermutation >= 0)
            {
                for (const auto& mesh : coll->Meshes)
                {
                    if (mesh.RegionIndex == static_cast<std::int16_t>(region) &&
                        mesh.PermutationIndex == defaultPermutation)
                    {
                        hasDefault = true;
                        break;
                    }
                }
            }

            std::vector<NodeBounds> perNode(coll->Nodes.size());

            for (const auto& mesh : coll->Meshes)
            {
                if (mesh.RegionIndex != static_cast<std::int16_t>(region)) continue;
                if (hasDefault && mesh.PermutationIndex != defaultPermutation) continue;
                if (mesh.NodeIndex < 0 ||
                    static_cast<std::size_t>(mesh.NodeIndex) >= perNode.size()) continue;

                NodeBounds& bounds = perNode[mesh.NodeIndex];
                if (!bounds.IsUsed)
                {
                    bounds = { true, mesh.LocalMin.X, mesh.LocalMin.Y, mesh.LocalMin.Z,
                                     mesh.LocalMax.X, mesh.LocalMax.Y, mesh.LocalMax.Z };
                    continue;
                }
                bounds.MinX = (std::min)(bounds.MinX, mesh.LocalMin.X);
                bounds.MinY = (std::min)(bounds.MinY, mesh.LocalMin.Y);
                bounds.MinZ = (std::min)(bounds.MinZ, mesh.LocalMin.Z);
                bounds.MaxX = (std::max)(bounds.MaxX, mesh.LocalMax.X);
                bounds.MaxY = (std::max)(bounds.MaxY, mesh.LocalMax.Y);
                bounds.MaxZ = (std::max)(bounds.MaxZ, mesh.LocalMax.Z);
            }

            int bestNode{ -1 };
            float bestVolume{ -1.0f };
            for (std::size_t node = 0; node < perNode.size(); ++node)
            {
                const NodeBounds& bounds = perNode[node];
                if (!bounds.IsUsed) continue;
                if (node >= nodeMap.size() || nodeMap[node] < 0) continue;

                const float volume = (bounds.MaxX - bounds.MinX) * (bounds.MaxY - bounds.MinY) * (bounds.MaxZ - bounds.MinZ);
                if (volume > bestVolume)
                {
                    bestVolume = volume;
                    bestNode = static_cast<int>(node);
                }
            }
            if (bestNode < 0) continue;

            const NodeBounds& bounds = perNode[bestNode];
            Anchor& out = anchors[region];
            out.ModelNodeIndex = nodeMap[bestNode];
            out.LocalOffset.X = (bounds.MinX + bounds.MaxX) * 0.5f;
            out.LocalOffset.Y = (bounds.MinY + bounds.MaxY) * 0.5f;
            out.LocalOffset.Z = (bounds.MinZ + bounds.MaxZ) * 0.5f;
            out.Radius = (std::max)({ bounds.MaxX - bounds.MinX, bounds.MaxY - bounds.MinY, bounds.MaxZ - bounds.MinZ }) * 0.5f;
            out.Source = AnchorSource::CollRegion;
        }

        return anchors;
    }

    auto ModelLinkBuilder::BuildObjectCenter(const Mode* mode) const -> Anchor
    {
        Anchor out{};
        if (!mode) return out;

        const auto& bounds = mode->ModelBounds;
        const float sizeX = bounds.Max.X - bounds.Min.X;
        const float sizeY = bounds.Max.Y - bounds.Min.Y;
        const float sizeZ = bounds.Max.Z - bounds.Min.Z;
        if (sizeX <= 0.0f && sizeY <= 0.0f && sizeZ <= 0.0f) return out;

        out.LocalOffset.X = (bounds.Min.X + bounds.Max.X) * 0.5f;
        out.LocalOffset.Y = (bounds.Min.Y + bounds.Max.Y) * 0.5f;
        out.LocalOffset.Z = (bounds.Min.Z + bounds.Max.Z) * 0.5f;
        out.Radius = (std::max)({ sizeX, sizeY, sizeZ }) * 0.5f;
        out.Source = AnchorSource::ObjectCenter;

        return out;
    }
}