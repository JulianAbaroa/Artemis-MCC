#pragma once

#include <cstdint>
#include <vector>

struct Coll_Regions_Permutations_BspsObject;
struct CollGeometry;
struct CollObject;
struct CollVec3;
struct Vec3;

class System_Logs;

struct Sys_CollBuilder_Deps
{
	System_Logs& System_Logs;
};

class System_Logs;

class System_CollBuilder
{
public:
	System_CollBuilder(Sys_CollBuilder_Deps deps) : m_Deps(deps) {}
	~System_CollBuilder() = default;

	CollGeometry Build(const CollObject& coll);

private:
	Sys_CollBuilder_Deps m_Deps;

	void BuildMeshes(const CollObject& coll, CollGeometry& out);
	void BuildNodes(const CollObject& coll, CollGeometry& out);
	void BuildBounds(CollGeometry& out);

	// --- Helpers ---
	CollVec3 MakeVec3(const Vec3& v);
	std::vector<int32_t> CollectSurfaceVertexIndices(
		const Coll_Regions_Permutations_BspsObject& bsp,
		int32_t surfaceIndex);
};