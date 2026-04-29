#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Shape primitives, derived from BspPhysics.ShapeType.
enum class CollShapeType : uint8_t
{
	Unknown = 0,
	Sphere,
	Pill,
	Box,
	MOPP,
};

struct CollVec3 { float X, Y, Z; };

// One primitive shape per Region/Permutation pair.
// Corresponds to one Coll_Regions_Permutations_BspPhysicsEntry.
struct CollShape
{
	CollShapeType Type = CollShapeType::Unknown;

	// Local-space center (from BspPhysics.Center).
	CollVec3 Center = {};

	// Box: half-extents. Sphere/Pill: X = radius, Y = half-height (Pill only).
	CollVec3 HalfExtent = {};

	// Uniform scale applied on top of the shape (from BspPhysics.Scale).
	float Scale = 1.0f;
};

// Pathfinding sphere, used by Navigation to mark passable/impassable zones.
struct CollPathfindingSphere
{
	CollVec3 Center = {};
	float Radius = 0.0f;

	bool VehicleOnly = false;
	bool WithSectors = false;

	// Which bone this sphere is attached to.
	int16_t NodeIndex = -1;
};

// One node in the collision skeleton hierarchy.
struct CollNode
{
	std::string Name;
	int16_t ParentIndex = -1;
	int16_t NextSiblingIndex = -1;
	int16_t FirstChildIndex = -1;
};

// One region with its permutations' shapes.
// A region typically maps to a destructible part of the object
// (e.g. "body", "left_door"). Permutations are damage states.
struct CollRegion
{
	std::string Name;

	// Index 0 = default/undamaged permutation.
	// Each permutation can have multiple shapes (one per BspPhysics entry).
	std::vector<std::vector<CollShape>> PermutationShapes;
};

// Top-level processed collision geometry.
// Equivalent to SbspGeometry for the coll domain.
struct CollGeometry
{
	std::string TagName;

	// Pathfinding spheres (used by Navigation and Interactable).
	std::vector<CollPathfindingSphere> PathfindingSpheres;

	// Collision skeleton — needed to resolve NodeIndex in spheres
	// and later for bone-space transforms.
	std::vector<CollNode> Nodes;

	// Convenience: AABB of all shapes across all regions/permutations.
	// Computed once by CollShapeBuilder, used as fast broad-phase bounds.
	CollVec3 BoundsMin = {};
	CollVec3 BoundsMax = {};
};