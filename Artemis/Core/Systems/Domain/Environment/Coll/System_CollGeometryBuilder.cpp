#include "pch.h"
#include "Core/Systems/Domain/Environment/Coll/System_CollGeometryBuilder.h"
#include "Core/Types/Domain/Domains/Environment/CollGeometry.h"
#include "Generated/Coll/CollObject.h"
#include <limits>

CollGeometry System_CollGeometryBuilder::BuildGeometry(const CollObject& coll)
{
	CollGeometry out;
	out.TagName = coll.TagName;

	this->BuildPathfindingSpheres(coll, out);
	this->BuildNodes(coll, out);
	this->BuildBounds(out);

	return out;
}

void System_CollGeometryBuilder::BuildPathfindingSpheres(const CollObject& coll, CollGeometry& out)
{
    out.PathfindingSpheres.reserve(coll.PathfindingSpheres.size());

    for (const auto& src : coll.PathfindingSpheres)
    {
        CollPathfindingSphere sphere;
        sphere.Center = MakeVec3(src.Center);
        sphere.Radius = src.Radius;
        sphere.NodeIndex = src.Node;
        sphere.VehicleOnly = (src.Flags & (1 << 1)) != 0;  // Vehicle Only = bit 1
        sphere.WithSectors = (src.Flags & (1 << 2)) != 0;  // With Sectors = bit 2

        out.PathfindingSpheres.push_back(sphere);
    }
}

void System_CollGeometryBuilder::BuildNodes(const CollObject& coll, CollGeometry& out)
{
    out.Nodes.reserve(coll.Nodes.size());

    for (const auto& src : coll.Nodes)
    {
        CollNode node;
        // TagName resolution from stringid goes here when available.
        // For now we store the raw stringid as a placeholder.
        node.Name = std::to_string(src.Name);
        node.ParentIndex = src.ParentNodeIndex;
        node.NextSiblingIndex = src.NextSiblingNodeIndex;
        node.FirstChildIndex = src.FirstChildNodeIndex;

        out.Nodes.push_back(node);
    }
}

void System_CollGeometryBuilder::BuildBounds(CollGeometry& out)
{
    if (out.PathfindingSpheres.empty())
    {
        out.BoundsMin = {};
        out.BoundsMax = {};
        return;
    }

    constexpr float kMax = (std::numeric_limits<float>::max)();
    out.BoundsMin = { kMax,  kMax,  kMax };
    out.BoundsMax = { -kMax, -kMax, -kMax };

    for (const auto& s : out.PathfindingSpheres)
    {
        out.BoundsMin.X = (std::min)(out.BoundsMin.X, s.Center.X - s.Radius);
        out.BoundsMin.Y = (std::min)(out.BoundsMin.Y, s.Center.Y - s.Radius);
        out.BoundsMin.Z = (std::min)(out.BoundsMin.Z, s.Center.Z - s.Radius);

        out.BoundsMax.X = (std::max)(out.BoundsMax.X, s.Center.X + s.Radius);
        out.BoundsMax.Y = (std::max)(out.BoundsMax.Y, s.Center.Y + s.Radius);
        out.BoundsMax.Z = (std::max)(out.BoundsMax.Z, s.Center.Z + s.Radius);
    }
}

CollVec3 System_CollGeometryBuilder::MakeVec3(const Vec3& v) { return { v.X, v.Y, v.Z }; }