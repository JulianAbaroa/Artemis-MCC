export module Resolved.World.Type:Sbsp;

import Common.Math.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using Vec4 = Common::Math::Type::Vec4;
	using Triangle = Common::Math::Type::Triangle;
}

export namespace Resolved::World::Type::Sbsp
{
	struct WorldBounds
	{
		Vec3 Min;
		Vec3 Max;
	};

	// Cross-BSP link
	//
	// Represents a connection from a cluster in this BSP to a cluster in another
	// BSP, discovered by matching Structure Seam centroids across BSPs.
	//
	// RemoteSbspIndex    
	// RemoteClusterIndex — index into that SbspGeometry::Clusters
	// ConnectionPoint    — world space position of the seam centroid
	//                      (useful for pathfinding cost estimation)
	struct CrossLink
	{
		std::int32_t  RemoteSbspIndex;
		std::int32_t  RemoteClusterIndex;
		Vec3 ConnectionPoint;
	};


	// Cluster (spatial cell)
	//
	// Each cluster is a convex region of the BSP.  The engine uses cluster
	// portals to determine visibility and audio propagation; for navigation we
	// use the same connectivity as a graph of reachable regions.
	//
	// BoundsMin/Max come from Sbsp_ClustersEntry::BoundsX/Y/Z (RangeF).
	// ConnectedClusters is built by walking every portal listed in Portals[]
	// and recording whichever of BackCluster/FrontCluster is not this cluster.
	// CrossLinks is populated after all BSPs are built, by SeamLinker.
	struct Cluster
	{
		std::int32_t ClusterIndex;          // index of this cluster in SbspObject::Clusters
		Vec3 BoundsMin;
		Vec3 BoundsMax;
		Vec3 Center;                // midpoint of bounds, useful for distance checks
		std::int16_t MeshIndex;             // render mesh index (-1 if none)

		// Intra-BSP adjacency (via portals, same BSP)
		std::vector<std::int32_t> ConnectedClusters;

		// Portal indices (into SbspGeometry::Portals) that touch this cluster
		std::vector<std::int32_t> PortalIndices;

		// Inter-BSP adjacency (via structure seams, different BSP)
		std::vector<CrossLink> CrossLinks;
	};


	// Portal (connection between two clusters)
	//
	// A portal is a convex polygon that lies on the boundary between two
	// adjacent clusters. For navigation, BackCluster and FrontCluster give
	// the pair, Centroid and BoundingRadius describe the opening.
	// Vertices give the exact polygon if needed for width/height checks.
	struct Portal
	{
		std::int32_t PortalIndex;
		std::int32_t BackCluster;           // index into SbspGeometry::Clusters (-1 = outside)
		std::int32_t FrontCluster;          // index into SbspGeometry::Clusters (-1 = outside)
		Vec3 Centroid;
		float BoundingRadius;

		std::vector<Vec3> Vertices; // polygon vertices (world space)
	};

	// Instanced geometry subgroup
	//
	// InstancedGeometryInstances holds only a Name (stringid).
	// InstancedGeometrySubGroups holds the spatial data and a Members list
	// whose NameIndex values index into InstancedGeometryInstances.
	//
	// For navigation we care about where a piece of instanced geometry sits
	// (Center, Radius) and what it is called (resolved NameId or index).
	struct InstancedGeometryEntry
	{
		std::int32_t  SubGroupIndex;         // index in SbspObject::InstancedGeometrySubGroups
		std::int32_t  NameIndex;             // index in SbspObject::InstancedGeometryInstances
		// (-1 if the subgroup has no members)
		std::uint32_t NameId;                // raw stringid from InstancedGeometryInstancesEntry
		// (0 if NameIndex == -1)
		Vec3 Center;                // SubGroupsEntry::Center
		float    Radius;                // SubGroupsEntry::Radius
		std::int16_t  ClusterCount;          // how many clusters this subgroup spans
	};

	// Collision material
	//
	// Describes a surface material used by the MOPP collision tree.
	// GlobalMaterialIndex is what the physics/game code uses to look up
	// friction, sound, etc.
	struct CollisionMaterial
	{
		std::int32_t  MaterialIndex;         // index in SbspObject::CollisionMaterials
		std::int16_t  GlobalMaterialIndex;
		std::int16_t  ConveyorSurfaceIndex;
		std::int16_t  SeamIndex;
	};

	// Marker
	//
	// Markers are named points in world space (spawn points, objective anchors,
	// etc.). Name is a stringid (uint32_t), it cannot be resolved to a string
	// without an external string table, so we keep it as NameId.
	// Rotation is stored as a quaternion (X,Y,Z,W).
	struct Marker
	{
		std::int32_t  MarkerIndex;           // index in SbspObject::Markers
		std::uint32_t NameId;                // stringid — opaque unless you have the string table
		Vec3 Position;
		Vec4 Rotation;              // quaternion (X,Y,Z,W)
	};

	struct Sbsp
	{
		std::string TagName;
		std::int32_t SbspIndex = -1;

		WorldBounds WorldBounds;

		Vec3 MoppBoundsMin;
		Vec3 MoppBoundsMax;

		std::vector<InstancedGeometryEntry> InstancedGeometry;
		std::vector<Cluster> Clusters;
		std::vector<Portal> Portals;

		std::vector<CollisionMaterial> CollisionMaterials;
		std::vector<Marker> Markers;
		std::vector<Triangle> RenderGeometry;

		std::int32_t ClusterCount() const
		{
			return static_cast<std::int32_t>(Clusters.size());
		}

		std::int32_t FindClusterForPoint(float x, float y, float z) const
		{
			std::int32_t bestCluster = -1;
			float bestVolume = (std::numeric_limits<float>::max)();

			for (const auto& c : Clusters)
			{
				if (x >= c.BoundsMin.X && x <= c.BoundsMax.X &&
					y >= c.BoundsMin.Y && y <= c.BoundsMax.Y &&
					z >= c.BoundsMin.Z && z <= c.BoundsMax.Z)
				{
					float vol = (c.BoundsMax.X - c.BoundsMin.X) *
						(c.BoundsMax.Y - c.BoundsMin.Y) *
						(c.BoundsMax.Z - c.BoundsMin.Z);

					if (vol < bestVolume)
					{
						bestVolume = vol;
						bestCluster = c.ClusterIndex;
					}
				}
			}

			return bestCluster;
		}
	};
}