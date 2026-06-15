#include "pch.h"

#include "System_CollBuilder.h"

#include "Core/Types/Sources/Static/World/CollGeometry.h"

#include "Core/Systems/Other/Logs/System_Logs.h"

#include "Generated/Coll/CollObject.h"

CollGeometry System_CollBuilder::Build(const CollObject& coll)
{
	CollGeometry out;
	out.TagName = coll.TagName;

	this->BuildNodes(coll, out);
	this->BuildMeshes(coll, out);
	this->BuildBounds(out);

	return out;
}

void System_CollBuilder::BuildNodes(
	const CollObject& coll, CollGeometry& out)
{
	out.Nodes.reserve(coll.Nodes.size());

	for (const auto& src : coll.Nodes)
	{
		CollNode node;
		node.Name = std::to_string(src.Name);
		node.ParentIndex = src.ParentNodeIndex;
		node.NextSiblingIndex = src.NextSiblingNodeIndex;
		node.FirstChildIndex = src.FirstChildNodeIndex;

		out.Nodes.push_back(node);
	}
}

void System_CollBuilder::BuildMeshes(
	const CollObject& coll, CollGeometry& out)
{
	for (int16_t i = 0; 
		i < static_cast<int16_t>(coll.Regions.size());
		++i)
	{
		const auto& region = coll.Regions[i];

		for (int16_t j = 0;
			j < static_cast<int16_t>(region.Permutations.size());
			++j)
		{
			const auto& permutation = region.Permutations[j];

			for (const auto& bsp : permutation.Bsps)
			{
				CollMesh mesh;
				mesh.NodeIndex = bsp.NodeIndex;
				mesh.RegionIndex = i;
				mesh.PermutationIndex = j;

				mesh.Triangles.reserve(bsp.Surfaces.size());

				for (int32_t k = 0;
					k < static_cast<int32_t>(bsp.Surfaces.size()); 
					++k)
				{
					const std::vector<int32_t> indices = 
						this->CollectSurfaceVertexIndices(bsp, k);

					if (indices.size() < 3) continue;

					const auto& surface = bsp.Surfaces[k];

					for (size_t i = 1; i + 1 < indices.size(); ++i)
					{
						CollTriangle triangle;

						triangle.A = this->MakeVec3(
							bsp.Vertices[indices[0]].Point);

						triangle.B = this->MakeVec3(
							bsp.Vertices[indices[i]].Point);

						triangle.C = this->MakeVec3(
							bsp.Vertices[indices[i + 1]].Point);

						triangle.SurfaceFlags = surface.Flags;
						triangle.Material = surface.Material;
						mesh.Triangles.push_back(triangle);
					}
				}

				if (mesh.Triangles.empty()) continue;

				constexpr float kMax = 
					(std::numeric_limits<float>::max)();

				mesh.LocalMin = { kMax,  kMax,  kMax };
				mesh.LocalMax = { -kMax, -kMax, -kMax };

				auto expandBounds = [&](const CollVec3& q) {
					mesh.LocalMin.X = (std::min)(mesh.LocalMin.X, q.X);
					mesh.LocalMin.Y = (std::min)(mesh.LocalMin.Y, q.Y);
					mesh.LocalMin.Z = (std::min)(mesh.LocalMin.Z, q.Z);
					mesh.LocalMax.X = (std::max)(mesh.LocalMax.X, q.X);
					mesh.LocalMax.Y = (std::max)(mesh.LocalMax.Y, q.Y);
					mesh.LocalMax.Z = (std::max)(mesh.LocalMax.Z, q.Z);
				};

				for (const auto& t : mesh.Triangles) 
				{ 
					expandBounds(t.A); 
					expandBounds(t.B); 
					expandBounds(t.C); 
				}

				out.Meshes.push_back(std::move(mesh));
			}
		}
	}

	out.RegionNames.assign(coll.Regions.size(), 0);
	out.PermutationNames.assign(coll.Regions.size(), {});
	out.DefaultPermutationIndex.assign(coll.Regions.size(), -1);

	for (size_t region = 0; region < coll.Regions.size(); ++region)
	{
		out.RegionNames[region] = coll.Regions[region].Name;

		const auto& permutations = coll.Regions[region].Permutations;
		out.PermutationNames[region].resize(permutations.size());

		for (size_t i = 0; i < permutations.size(); ++i)
		{
			out.PermutationNames[region][i] = permutations[i].Name;

			// 0x0001 is "default" on the StringTable.
			if (permutations[i].Name == 0x0001)
			{
				out.DefaultPermutationIndex[region] = (int)i;
			}
		}
	}
}

void System_CollBuilder::BuildBounds(CollGeometry& out)
{
	if (out.Meshes.empty())
	{
		out.BoundsMin = {};
		out.BoundsMax = {};
		return;
	}

	constexpr float kMax = (std::numeric_limits<float>::max)();
	out.BoundsMin = { kMax,  kMax,  kMax };
	out.BoundsMax = { -kMax, -kMax, -kMax };

	for (const auto& m : out.Meshes)
	{
		out.BoundsMin.X = (std::min)(out.BoundsMin.X, m.LocalMin.X);
		out.BoundsMin.Y = (std::min)(out.BoundsMin.Y, m.LocalMin.Y);
		out.BoundsMin.Z = (std::min)(out.BoundsMin.Z, m.LocalMin.Z);
		out.BoundsMax.X = (std::max)(out.BoundsMax.X, m.LocalMax.X);
		out.BoundsMax.Y = (std::max)(out.BoundsMax.Y, m.LocalMax.Y);
		out.BoundsMax.Z = (std::max)(out.BoundsMax.Z, m.LocalMax.Z);
	}
}

// --- Helpers ---

CollVec3 System_CollBuilder::MakeVec3(const Vec3& v) 
{ 
	return { v.X, v.Y, v.Z }; 
}

std::vector<int32_t> System_CollBuilder::CollectSurfaceVertexIndices(
	const Coll_Regions_Permutations_BspsObject& bsp,
	int32_t surfaceIndex)
{
	std::vector<int32_t> ring;

	const auto& surfaces = bsp.Surfaces;
	const auto& edges = bsp.Edges;

	if (surfaceIndex < 0 ||
		static_cast<size_t>(surfaceIndex) >= surfaces.size())
	{
		return ring;
	}

	const int32_t firstEdge = surfaces[surfaceIndex].FirstEdge;

	if (firstEdge < 0 ||
		static_cast<size_t>(firstEdge) >= edges.size())
	{
		return ring;
	}

	int32_t i = firstEdge;

	const size_t maxSteps = edges.size() + 1;
	size_t steps = 0;

	do
	{
		if (i < 0 || static_cast<size_t>(i) >= edges.size())
		{
			ring.clear();
			return ring;
		}

		const auto& edge = edges[i];

		int32_t vertex;
		int32_t next;

		if (edge.LeftSurface == surfaceIndex)
		{
			vertex = edge.StartVertex;
			next = edge.ForwardEdge;
		}
		else
		{
			vertex = edge.EndVertex;
			next = edge.ReverseEdge;
		}

		if (vertex < 0 ||
			static_cast<size_t>(vertex) >= bsp.Vertices.size())
		{
			ring.clear();
			return ring;
		}

		ring.push_back(vertex);
		i = next;

		if (++steps > maxSteps)
		{
			ring.clear();
			return ring;
		}
	} while (i != firstEdge);

	return ring;
}