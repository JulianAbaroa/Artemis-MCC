#pragma once

// Core/Common/Types/Domain/Map/SbspGeometry.h
//
// Processed representation of SBSP data for AI navigation.
// These structs are produced by SbspGeometryBuilder::BuildGeometry()
// from a fully-deserialized SbspObject.

#include <cstdint>
#include <string>
#include <vector>

// Primitive types (independent of MapTypes.h so AI code has no engine deps).
struct SbspVec2 { float X, Y; };
struct SbspVec3 { float X, Y, Z; };
struct SbspVec4 { float X, Y, Z, W; };

// World bounds.
struct SbspWorldBounds
{
    SbspVec3 Min;
    SbspVec3 Max;
};

// Cluster (spatial cell)
//
// Each cluster is a convex region of the BSP. The engine uses cluster
// portals to determine visibility and audio propagation, for navigation we
// use the same connectivity as a graph of reachable regions.
//
// BoundsMin/Max come from Sbsp_ClustersEntry::BoundsX/Y/Z (RangeF).
// ConnectedClusters is built by walking every portal listed in Portals[]
// and recording whichever of BackCluster/FrontCluster is not this cluster.
struct SbspCluster
{
    int32_t  ClusterIndex;          // index of this cluster in SbspObject::Clusters
    SbspVec3 BoundsMin;
    SbspVec3 BoundsMax;
    SbspVec3 Center;                // midpoint of bounds, useful for distance checks
    int16_t  MeshIndex;             // render mesh index (-1 if none)

    // Adjacency list: indices into SbspGeometry::Clusters.
    std::vector<int32_t> ConnectedClusters;

    // Portal indices (into SbspGeometry::Portals) that touch this cluster.
    std::vector<int32_t> PortalIndices;
};

// Portal (connection between two clusters)
//
// A portal is a convex polygon that lies on the boundary between two
// adjacent clusters. For navigation, BackCluster and FrontCluster give
// the pair, Centroid and BoundingRadius describe the opening.
// Vertices give the exact polygon if needed for width/height checks.
struct SbspPortal
{
    int32_t  PortalIndex;
    int32_t  BackCluster;           // index into SbspGeometry::Clusters (-1 = outside)
    int32_t  FrontCluster;          // index into SbspGeometry::Clusters (-1 = outside)
    SbspVec3 Centroid;
    float    BoundingRadius;

    std::vector<SbspVec3> Vertices; // polygon vertices (world space)
};

// Instanced geometry subgroup
//
// InstancedGeometryInstances holds only a Name (stringid).
// InstancedGeometrySubGroups holds the spatial data and a Members list
// whose NameIndex values index into InstancedGeometryInstances.
//
// For navigation we care about where a piece of instanced geometry sits
// (Center, Radius) and what it is called (resolved NameId or index).
struct SbspInstancedGeometryEntry
{
    int32_t  SubGroupIndex;         // index in SbspObject::InstancedGeometrySubGroups
    int32_t  NameIndex;             // index in SbspObject::InstancedGeometryInstances
    // (-1 if the subgroup has no members)
    uint32_t NameId;                // raw stringid from InstancedGeometryInstancesEntry
    // (0 if NameIndex == -1)
    SbspVec3 Center;                // SubGroupsEntry::Center
    float    Radius;                // SubGroupsEntry::Radius
    int16_t  ClusterCount;          // how many clusters this subgroup spans
};

// Collision material
//
// Describes a surface material used by the MOPP collision tree.
// GlobalMaterialIndex is what the physics/game code uses to look up
// friction, sound, etc.
struct SbspCollisionMaterial
{
    int32_t  MaterialIndex;         // index in SbspObject::CollisionMaterials
    int16_t  GlobalMaterialIndex;
    int16_t  ConveyorSurfaceIndex;
    int16_t  SeamIndex;
};

// Marker
//
// Markers are named points in world space (spawn points, objective anchors,
// etc.). Name is a stringid (uint32_t), it cannot be resolved to a string
// without an external string table, so we keep it as NameId.
// Rotation is stored as a quaternion (X,Y,Z,W).
struct SbspMarker
{
    int32_t  MarkerIndex;           // index in SbspObject::Markers
    uint32_t NameId;                // stringid — opaque unless you have the string table
    SbspVec3 Position;
    SbspVec4 Rotation;              // quaternion (X,Y,Z,W)
};

// Top-level processed geometry struct.
struct SbspGeometry
{
    std::string TagName;            // copied from SbspObject::TagName

    // World extents
    SbspWorldBounds WorldBounds;

    // MOPP collision BVH extents (may differ from WorldBounds)
    SbspVec3 MoppBoundsMin;
    SbspVec3 MoppBoundsMax;

    // Cluster graph — index i corresponds to Clusters[i].ClusterIndex == i
    std::vector<SbspCluster>   Clusters;

    // Portal list — index i corresponds to Portals[i].PortalIndex == i
    std::vector<SbspPortal>    Portals;

    // Instanced geometry (one entry per SubGroup that has at least one member)
    std::vector<SbspInstancedGeometryEntry> InstancedGeometry;

    // Collision materials
    std::vector<SbspCollisionMaterial> CollisionMaterials;

    // Map markers (spawn points, objectives, etc.)
    std::vector<SbspMarker> Markers;

    // Convenience: number of clusters (== Clusters.size())
    int32_t ClusterCount() const { return static_cast<int32_t>(Clusters.size()); }

    // Convenience: find which cluster contains a world-space point (AABB test).
    // Returns -1 if no cluster's bounding box contains the point.
    // Note: clusters can overlap; returns the first match.
    int32_t FindClusterForPoint(float x, float y, float z) const
    {
        for (const auto& c : Clusters)
        {
            if (x >= c.BoundsMin.X && x <= c.BoundsMax.X &&
                y >= c.BoundsMin.Y && y <= c.BoundsMax.Y &&
                z >= c.BoundsMin.Z && z <= c.BoundsMax.Z)
            {
                return c.ClusterIndex;
            }
        }
        return -1;
    }
};