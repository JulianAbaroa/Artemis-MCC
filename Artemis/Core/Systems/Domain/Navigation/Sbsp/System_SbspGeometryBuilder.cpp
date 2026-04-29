#include "pch.h"
#include "Core/Systems/Domain/Navigation/Sbsp/System_SbspGeometryBuilder.h"

// --- BuildGeometry, orchestrates all sub-builders in dependency order ---

SbspGeometry System_SbspGeometryBuilder::BuildGeometry(const SbspObject& sbsp)
{
    SbspGeometry out;
    out.TagName = sbsp.TagName;

    this->BuildWorldBounds(sbsp, out);
    this->BuildPortals(sbsp, out);          // must come before BuildClusters
    this->BuildClusters(sbsp, out);         // must come before BuildClusterAdjacency
    this->BuildClusterAdjacency(sbsp, out); // requires Clusters + Portals populated
    this->BuildInstancedGeometry(sbsp, out);
    this->BuildCollisionMaterials(sbsp, out);
    this->BuildMarkers(sbsp, out);

    return out;
}

// --- World bounds ---

void System_SbspGeometryBuilder::BuildWorldBounds(const SbspObject& sbsp, SbspGeometry& out)
{
    const SbspData& d = sbsp.Data;

    out.WorldBounds.Min = { d.WorldBoundsX.Min, d.WorldBoundsY.Min, d.WorldBoundsZ.Min };
    out.WorldBounds.Max = { d.WorldBoundsX.Max, d.WorldBoundsY.Max, d.WorldBoundsZ.Max };

    out.MoppBoundsMin = this->MakeVec3(d.MoppBoundsMinimum);
    out.MoppBoundsMax = this->MakeVec3(d.MoppBoundsMaximum);
}

// Portals
//
// Each Sbsp_ClusterPortalsObject gives us:
//   BackCluster  / FrontCluster  — the two clusters on each side
//   Centroid     / BoundingRadius — spatial description of the opening
//   Vertices[]   — exact polygon (world space)
//
// BackCluster / FrontCluster can be -1 (0xFFFF as int16 → sign-extended)
// which means the portal borders the void / exterior.  We keep those as -1.
void System_SbspGeometryBuilder::BuildPortals(const SbspObject& sbsp, SbspGeometry& out)
{
    const int32_t count = static_cast<int32_t>(sbsp.ClusterPortals.size());
    out.Portals.reserve(count);

    for (int32_t i = 0; i < count; ++i)
    {
        const Sbsp_ClusterPortalsObject& src = sbsp.ClusterPortals[i];

        SbspPortal p;
        p.PortalIndex = i;
        p.BackCluster = static_cast<int32_t>(src.BackCluster);
        p.FrontCluster = static_cast<int32_t>(src.FrontCluster);
        p.Centroid = this->MakeVec3(src.Centroid);
        p.BoundingRadius = src.BoundingRadius;

        p.Vertices.reserve(src.Vertices.size());
        for (const auto& v : src.Vertices)
            p.Vertices.push_back(this->MakeVec3(v.Position));

        out.Portals.push_back(std::move(p));
    }
}

// Clusters
//
// Each Sbsp_ClustersObject gives us:
//   BoundsX/Y/Z  — AABB of the cluster (RangeF)
//   MeshIndex    — which render mesh this cluster uses (-1 = none)
//   Portals[]    — list of Sbsp_Clusters_PortalsEntry, each with a PortalIndex
//                  that indexes into ClusterPortals (and therefore out.Portals)
//
// We build the full SbspCluster here; adjacency edges are added in the next
// pass so that all SbspCluster objects already exist when we link them.
void System_SbspGeometryBuilder::BuildClusters(const SbspObject& sbsp, SbspGeometry& out)
{
    const int32_t count = static_cast<int32_t>(sbsp.Clusters.size());
    out.Clusters.reserve(count);

    for (int32_t i = 0; i < count; ++i)
    {
        const Sbsp_ClustersObject& src = sbsp.Clusters[i];

        SbspCluster c;
        c.ClusterIndex = i;
        c.BoundsMin = { src.BoundsX.Min, src.BoundsY.Min, src.BoundsZ.Min };
        c.BoundsMax = { src.BoundsX.Max, src.BoundsY.Max, src.BoundsZ.Max };
        c.Center = this->RangeFCenter(src.BoundsX, src.BoundsY, src.BoundsZ);
        c.MeshIndex = src.MeshIndex;

        // Record which portals touch this cluster
        c.PortalIndices.reserve(src.Portals.size());
        for (const auto& pe : src.Portals)
            c.PortalIndices.push_back(static_cast<int32_t>(pe.PortalIndex));

        out.Clusters.push_back(std::move(c));
    }
}

// Cluster adjacency
//
// Walk every portal this cluster references.  For each portal, whichever of
// BackCluster / FrontCluster is not this cluster is a neighbor.
// We deduplicate because two portals can share the same pair of clusters.
//
// Validity guards:
//   - portal index must be in range
//   - neighbor index must be >= 0 and < cluster count
//   - neighbor must not be the cluster itself
void System_SbspGeometryBuilder::BuildClusterAdjacency(const SbspObject& sbsp, SbspGeometry& out)
{
    const int32_t clusterCount = static_cast<int32_t>(out.Clusters.size());
    const int32_t portalCount = static_cast<int32_t>(out.Portals.size());

    for (int32_t ci = 0; ci < clusterCount; ++ci)
    {
        SbspCluster& cluster = out.Clusters[ci];

        for (int32_t portalIdx : cluster.PortalIndices)
        {
            if (portalIdx < 0 || portalIdx >= portalCount)
                continue;

            const SbspPortal& portal = out.Portals[portalIdx];

            // Check both sides of the portal; take whichever is not us
            for (int32_t neighbor : { portal.BackCluster, portal.FrontCluster })
            {
                if (neighbor < 0 || neighbor >= clusterCount || neighbor == ci)
                    continue;

                // Deduplicate — clusters rarely have > ~8 portals so linear is fine
                bool alreadyAdded = false;
                for (int32_t existing : cluster.ConnectedClusters)
                {
                    if (existing == neighbor)
                    {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (!alreadyAdded)
                    cluster.ConnectedClusters.push_back(neighbor);
            }
        }
    }
}

// Instanced geometry
//
// The data model is a three-level hierarchy:
//
//   InstancedGeometryGroups[g]
//       .Members[m] → SubGroupIndex
//           → InstancedGeometrySubGroups[s]
//               .Center, .Radius, .ClusterCount
//               .Members[k] → NameIndex
//                   → InstancedGeometryInstances[n].Name (stringid)
//
// For navigation purposes, one SbspInstancedGeometryEntry per SubGroup
// is enough — it gives position + rough extent.  We pick the first member's
// NameIndex as the representative name for the subgroup.
//
// SubGroups that have no Members entries (empty members list) are still
// included with NameIndex = -1 and NameId = 0.
void System_SbspGeometryBuilder::BuildInstancedGeometry(const SbspObject& sbsp, SbspGeometry& out)
{
    const int32_t instanceCount = static_cast<int32_t>(sbsp.InstancedGeometryInstances.size());
    const int32_t subGroupCount = static_cast<int32_t>(sbsp.InstancedGeometrySubGroups.size());

    out.InstancedGeometry.reserve(subGroupCount);

    for (int32_t si = 0; si < subGroupCount; ++si)
    {
        const Sbsp_InstancedGeometrySubGroupsObject& sg = sbsp.InstancedGeometrySubGroups[si];

        SbspInstancedGeometryEntry entry{};
        entry.SubGroupIndex = si;
        entry.Center = this->MakeVec3(sg.Center);
        entry.Radius = sg.Radius;
        entry.ClusterCount = sg.ClusterCount;
        entry.NameIndex = -1;
        entry.NameId = 0;

        // Take the first member's name as the representative name
        if (!sg.Members.empty())
        {
            const int32_t nameIdx = static_cast<int32_t>(sg.Members[0].NameIndex);
            if (nameIdx >= 0 && nameIdx < instanceCount)
            {
                entry.NameIndex = nameIdx;
                entry.NameId = sbsp.InstancedGeometryInstances[nameIdx].Name;
            }
        }

        out.InstancedGeometry.push_back(entry);
    }
}

// Collision materials
//
// Sbsp_CollisionMaterialsEntry maps a shader tag to a global material index.
// The global material index is what gameplay code (physics, audio, AI
// surface reactions) uses — that's the field most relevant to the agent.
void System_SbspGeometryBuilder::BuildCollisionMaterials(const SbspObject& sbsp, SbspGeometry& out)
{
    const int32_t count = static_cast<int32_t>(sbsp.CollisionMaterials.size());
    out.CollisionMaterials.reserve(count);

    for (int32_t i = 0; i < count; ++i)
    {
        const Sbsp_CollisionMaterialsEntry& src = sbsp.CollisionMaterials[i];

        SbspCollisionMaterial m;
        m.MaterialIndex = i;
        m.GlobalMaterialIndex = src.GlobalMaterialIndex;
        m.ConveyorSurfaceIndex = src.ConveyorSurfaceIndex;
        m.SeamIndex = src.SeamIndex;

        out.CollisionMaterials.push_back(m);
    }
}

// Markers
//
// Sbsp_MarkersEntry has:
//   Name     (uint32_t, stringid — opaque without a string table)
//   Position (Vec3)
//   Rotation (Vec4, quaternion stored as X,Y,Z,W)
//
// The ascii field is stored as uint32_t in the generated struct (4 bytes
// only) so we expose the stringid directly rather than attempt to recover
// a string.  If a string table resolver becomes available later, it can
// convert NameId to a std::string externally.
void System_SbspGeometryBuilder::BuildMarkers(const SbspObject& sbsp, SbspGeometry& out)
{
    const int32_t count = static_cast<int32_t>(sbsp.Markers.size());
    out.Markers.reserve(count);

    for (int32_t i = 0; i < count; ++i)
    {
        const Sbsp_MarkersEntry& src = sbsp.Markers[i];

        SbspMarker marker{};
        marker.MarkerIndex = i;
        marker.NameId = src.Name;
        marker.Position = this->MakeVec3(src.Position);
        marker.Rotation = this->MakeVec4(src.Rotation);

        out.Markers.push_back(marker);
    }
}