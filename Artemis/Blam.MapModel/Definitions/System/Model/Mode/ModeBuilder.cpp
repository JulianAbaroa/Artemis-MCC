module Resolved.Definitions.System;
import :Mode;

import Common.Math.Type;

namespace
{
    using Common::Math::Type::k_FloatMax;

    // Bit 0 of the compression flags of the compression info: compressed position.
    constexpr std::uint16_t k_CompressedPositionFlag{ 1u << 0 };
}

namespace Resolved::Definitions::System
{
    auto ModeBuilder::Build(const ModeObject& mode) -> Mode
    {
        Mode out{};

        out.TagName = mode.TagName;

        this->BuildMarkerGroups(mode, out);
        this->BuildNodes(mode, out);
        this->BuildRegions(mode, out);
        this->BuildBounds(mode, out);

        return out;
    }

    auto ModeBuilder::BuildMarkerGroups(const ModeObject& mode, Mode& out) -> void
    {
        out.MarkerGroups.reserve(mode.MarkerGroups.size());

        for (const auto& srcGroup : mode.MarkerGroups)
        {
            MarkerGroup group{};

            group.NameId = srcGroup.Name;
            group.Markers.reserve(srcGroup.Markers.size());

            for (const auto& srcMarker : srcGroup.Markers)
            {
                Marker marker{};

                marker.NodeIndex = srcMarker.NodeIndex;
                marker.Flags = srcMarker.Flags;
                marker.Translation = srcMarker.Translation;
                marker.Rotation = srcMarker.Rotation;
                marker.Direction = srcMarker.Direction;
                marker.Scale = srcMarker.Scale;

                group.Markers.push_back(marker);
            }

            out.MarkerGroups.push_back(std::move(group));
        }
    }

    auto ModeBuilder::BuildNodes(const ModeObject& mode, Mode& out) -> void
    {
        out.Nodes.reserve(mode.Nodes.size());

        for (const auto& src : mode.Nodes)
        {
            Node node{};

            node.Name = src.Name;
            node.ParentIndex = src.ParentNodeIndex;
            node.FirstChildIndex = src.FirstChildNodeIndex;
            node.NextSiblingIndex = src.NextSiblingNodeIndex;
            node.DefaultTranslation = src.DefaultTranslation;
            node.DefaultRotation = src.DefaultRotation;
            node.InverseScale = src.InverseScale;

            out.Nodes.push_back(node);
        }
    }

    auto ModeBuilder::BuildRegions(const ModeObject& mode, Mode& out) -> void
    {
        out.Regions.reserve(mode.Regions.size());

        for (const auto& src : mode.Regions)
        {
            Region region{};

            region.NameId = src.Name;
            region.PermutationNames.reserve(src.Permutations.size());
            region.PermutationMeshCounts.reserve(src.Permutations.size());

            for (const auto& permutation : src.Permutations)
            {
                region.PermutationNames.push_back(permutation.Name);
                region.PermutationMeshCounts.push_back(permutation.MeshIndex < 0 ? 0 : permutation.MeshCount);
            }

            out.Regions.push_back(std::move(region));
        }
    }

    auto ModeBuilder::BuildBounds(const ModeObject& mode, Mode& out) -> void
    {
        Vec3 min{ k_FloatMax, k_FloatMax, k_FloatMax };
        Vec3 max{ -k_FloatMax, -k_FloatMax, -k_FloatMax };

        bool found{};

        for (const auto& ci : mode.CompressionInfo)
        {
            if ((ci.CompressionFlags & k_CompressedPositionFlag) == 0)
                continue;

            min.X = (std::min)(min.X, ci.PositionBoundsX.Min);
            min.Y = (std::min)(min.Y, ci.PositionBoundsY.Min);
            min.Z = (std::min)(min.Z, ci.PositionBoundsZ.Min);

            max.X = (std::max)(max.X, ci.PositionBoundsX.Max);
            max.Y = (std::max)(max.Y, ci.PositionBoundsY.Max);
            max.Z = (std::max)(max.Z, ci.PositionBoundsZ.Max);

            found = true;
        }

        out.ModelBounds = found ? Bounds{ min, max } : Bounds{};
    }
}