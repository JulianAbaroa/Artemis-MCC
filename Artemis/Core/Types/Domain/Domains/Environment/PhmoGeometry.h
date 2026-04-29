#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct PhmoVec3 { float X, Y, Z; };

// Mirrors RigidBodies.ShapeType enum.
enum class PhmoShapeType : uint8_t
{
	Unknown = 0,
	Sphere = 1,
	Pill = 2,
	Box = 3,
	Polyhedron = 4,
	MultiSphere = 5,
};

struct PhmoSphereShape
{
	PhmoVec3 Center;
	float Radius;
};

struct PhmoPillShape
{
	PhmoVec3    Bottom;
	PhmoVec3    Top;
	float       Radius;
};

struct PhmoBoxShape
{
	PhmoVec3    Center;
	PhmoVec3    HalfExtents;
	PhmoVec3    RotationI;
	PhmoVec3    RotationJ;
	PhmoVec3    RotationK;
};

struct PhmoPolyhedronShape
{
	PhmoVec3    AABBCenter;
	PhmoVec3    AABBHalfExtents;
};

struct PhmoMultiSphereShape
{
	struct Sphere 
	{ 
		PhmoVec3 Center; 
		float Radius; 
	};

	std::vector<Sphere> Spheres;
};

struct PhmoShape
{
	PhmoShapeType Type = PhmoShapeType::Unknown;

	PhmoSphereShape Sphere;
	PhmoPillShape Pill;
	PhmoBoxShape Box;
	PhmoPolyhedronShape Polyhedron;
	PhmoMultiSphereShape MultiSphere;
};

// Collision group mask, which categories of objects collide
// with this rigid body. Derived from RigidBodies.CollisionGroupMask.
enum class PhmoCollisionGroup : uint32_t
{
	Everything = (1u << 0),
	EnvironmentDefault = (1u << 1),
	EnvironmentOnly = (1u << 2),
	SmallCrate = (1u << 3),
	Crate = (1u << 4),
	HugeCrate = (1u << 5),
	Item = (1u << 6),
	Projectile = (1u << 7),
	Machine = (1u << 8),
	EarlyMoverMachine = (1u << 9),
	Creature = (1u << 10),
	Biped = (1u << 11),
	DeadBiped = (1u << 12),
	Vehicle = (1u << 15),
};

struct PhmoRigidBody
{
	// Index to the node (bone) to which it is attached.
	int16_t     NodeIndex = -1;

	// Region/permutation index, for damage states.
	int16_t     RegionIndex = -1;
	int16_t     PermutationIndex = -1;

	// Broad-phase: sphere in local-space that encloses all shapes.
	PhmoVec3    BoundingSphereOffset = {};
	float       BoundingSphereRadius = 0.0f;

	// Collision group mask of the rigid body.
	uint32_t    CollisionGroupMask = 0;

	// Resolved shapes (may be >1 if RigidBody pointed to a List).
	std::vector<PhmoShape> Shapes;
};

struct PhmoNode
{
	std::string Name;
	int16_t     ParentIndex = -1;
	int16_t     SiblingIndex = -1;
	int16_t     ChildIndex = -1;
	bool        DoesNotAnimate = false;
};

struct PhmoGeometry
{
	std::string TagName;

	// Rigid bodies with their shapes already resolved.
	std::vector<PhmoRigidBody>  RigidBodies;

	// Physical node hierarchy.
	std::vector<PhmoNode>       Nodes;

	// AABB global derived from all bounding spheres.
	PhmoVec3 BoundsMin = {};
	PhmoVec3 BoundsMax = {};
};