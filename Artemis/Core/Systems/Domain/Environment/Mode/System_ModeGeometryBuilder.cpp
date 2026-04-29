#include "pch.h"
#include "Core/Systems/Domain/Environment/Mode/System_ModeGeometryBuilder.h"

ModeGeometry System_ModeGeometryBuilder::BuildGeometry(const ModeObject& mode)
{
    ModeGeometry out;
    out.TagName = mode.TagName;

    this->BuildMarkerGroups(mode, out);
    this->BuildNodes(mode, out);
    this->BuildBounds(mode, out);

    return out;
}

void System_ModeGeometryBuilder::BuildMarkerGroups(const ModeObject& mode, ModeGeometry& out)
{
    out.MarkerGroups.reserve(mode.MarkerGroups.size());

    for (const auto& srcGroup : mode.MarkerGroups)
    {
        ModeMarkerGroup group;
        group.NameId = srcGroup.Name;
        group.Name = std::to_string(srcGroup.Name);
        group.Markers.reserve(srcGroup.Markers.size());

        for (const auto& srcMarker : srcGroup.Markers)
        {
            ModeMarker marker;
            marker.RegionIndex = srcMarker.RegionIndex;
            marker.PermutationIndex = srcMarker.PermutationIndex;
            marker.NodeIndex = srcMarker.NodeIndex;
            marker.Translation = this->MakeVec3(srcMarker.Translation);
            marker.Rotation = this->MakeVec4(srcMarker.Rotation);
            marker.Scale = srcMarker.Scale;

            group.Markers.push_back(marker);
        }

        out.MarkerGroups.push_back(std::move(group));
    }
}

void System_ModeGeometryBuilder::BuildNodes(const ModeObject& mode, ModeGeometry& out)
{
    out.Nodes.reserve(mode.Nodes.size());

    for (const auto& src : mode.Nodes)
    {
        ModeNode node;
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

void System_ModeGeometryBuilder::BuildBounds(const ModeObject& mode, ModeGeometry& out)
{
    if (mode.CompressionInfo.empty())
    {
        out.BoundsMin = {};
        out.BoundsMax = {};
        return;
    }

    // We take the first entry, in simple objects there is only one.
    const auto& ci = mode.CompressionInfo[0];

    out.BoundsMin = { ci.PositionBoundsX.Min, ci.PositionBoundsY.Min, ci.PositionBoundsZ.Min };
    out.BoundsMax = { ci.PositionBoundsX.Max, ci.PositionBoundsY.Max, ci.PositionBoundsZ.Max };
}