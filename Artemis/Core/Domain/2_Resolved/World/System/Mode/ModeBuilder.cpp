module Resolved.World.System;
import :Mode;

namespace
{
    using Resolved::World::Type::Constant::k_CompressedPositionFlag;
}

namespace Resolved::World::System
{
    auto ModeBuilder::Build(const ModeObject& mode) -> ResolvedMode
    {
        ResolvedMode out;
        out.TagName = mode.TagName;

        this->BuildMarkerGroups(mode, out);
        this->BuildNodes(mode, out);

        return out;
    }

    auto ModeBuilder::BuildMarkerGroups(const ModeObject& mode, ResolvedMode& out) -> void
    {
        out.MarkerGroups.reserve(mode.MarkerGroups.size());

        for (const auto& srcGroup : mode.MarkerGroups)
        {
            MarkerGroup group;
            group.NameId = srcGroup.Name;
            group.Markers.reserve(srcGroup.Markers.size());

            for (const auto& srcMarker : srcGroup.Markers)
            {
                Marker marker;
                marker.NodeIndex = srcMarker.NodeIndex;
                marker.Translation = this->MakeVec3(srcMarker.Translation);
                marker.Rotation = this->MakeVec4(srcMarker.Rotation);
                marker.Scale = srcMarker.Scale;

                group.Markers.push_back(marker);
            }

            out.MarkerGroups.push_back(std::move(group));
        }
    }

    auto ModeBuilder::BuildNodes(const ModeObject& mode, ResolvedMode& out) -> void
    {
        out.Nodes.reserve(mode.Nodes.size());

        for (const auto& src : mode.Nodes)
        {
            Node node;
            node.Name = std::to_string(src.Name);
            node.ParentIndex = src.ParentNodeIndex;
            node.FirstChildIndex = src.FirstChildNodeIndex;
            node.NextSiblingIndex = src.NextSiblingNodeIndex;
            node.DefaultTranslation = this->MakeVec3(src.DefaultTranslation);
            node.DefaultRotation = this->MakeVec4(src.DefaultRotation);
            node.InverseScale = src.InverseScale;

            out.Nodes.push_back(node);
        }
    }

    auto ModeBuilder::BuildBounds(const ModeObject& mode, ResolvedMode& out) -> void
    {
        Vec3 min{ std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
        Vec3 max{ -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

        bool found = false;

        for (const auto& ci : mode.CompressionInfo)
        {
            if ((ci.CompressionFlags & k_CompressedPositionFlag) == 0)
                continue;

            min.X = std::min(min.X, ci.PositionBoundsX.Min);
            min.Y = std::min(min.Y, ci.PositionBoundsY.Min);
            min.Z = std::min(min.Z, ci.PositionBoundsZ.Min);

            max.X = std::max(max.X, ci.PositionBoundsX.Max);
            max.Y = std::max(max.Y, ci.PositionBoundsY.Max);
            max.Z = std::max(max.Z, ci.PositionBoundsZ.Max);

            found = true;
        }

        out.ModelBounds = found ? Bounds{ min, max } : Bounds{};
    }

    auto ModeBuilder::MakeVec3(const Map::Reader::Type::Structure::Primitive::Vec3& v) -> Vec3
    {
        return { v.X, v.Y, v.Z };
    }

    auto ModeBuilder::MakeVec4(const Map::Reader::Type::Structure::Primitive::Vec4& v) -> Vec4
    {
        return { v.X, v.Y, v.Z, v.W };
    }
}