#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct CollVec3 { float X, Y, Z; };

struct CollTriangle
{
	CollVec3 A = {};
	CollVec3 B = {};
	CollVec3 C = {};

	uint8_t SurfaceFlags = 0;
	int16_t Material = -1;
};

struct CollNode
{
	std::string Name;
	int16_t ParentIndex = -1;
	int16_t NextSiblingIndex = -1;
	int16_t FirstChildIndex = -1;
};

struct CollMesh
{
	int16_t NodeIndex = -1;

	int16_t RegionIndex = -1;
	int16_t PermutationIndex = -1;

	std::vector<CollTriangle> Triangles;

	CollVec3 LocalMin = {};
	CollVec3 LocalMax = {};
};

struct CollGeometry
{
	std::string TagName;

	std::vector<CollNode> Nodes;

	std::vector<CollMesh> Meshes;

	std::vector<uint32_t> RegionNames;
	std::vector<std::vector<uint32_t>> PermutationNames;
	std::vector<int> DefaultPermutationIndex;

	CollVec3 BoundsMin = {};
	CollVec3 BoundsMax = {};
};