export module Resolved.World.Type:Phmo;

import Common.Math.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using Node = Common::Math::Type::Node;
}

export namespace Resolved::World::Type::Phmo
{
	enum class ShapeType : std::uint8_t
	{
		Unknown = 0,
		Sphere = 1,
		Pill = 2,
		Box = 3,
		Polyhedron = 4,
		MultiSphere = 5,
	};

	struct Sphere
	{
		Vec3 Center;
		float Radius;
	};

	struct Pill
	{
		Vec3 Bottom;
		Vec3 Top;
		float Radius;
	};

	struct Box
	{
		Vec3 Center;
		Vec3 HalfExtents;
		Vec3 RotationI;
		Vec3 RotationJ;
		Vec3 RotationK;
	};

	struct Polyhedron
	{
		Vec3 AABBCenter;
		Vec3 AABBHalfExtents;
		std::vector<Vec3> Vertices;
	};

	struct MultiSphere
	{
		struct Sphere
		{
			Vec3 Center;
			float Radius;
		};

		std::vector<Sphere> Spheres;
	};

	struct Shape
	{
		ShapeType Type = ShapeType::Unknown;

		Sphere Sphere;
		Pill Pill;
		Box Box;
		Polyhedron Polyhedron;
		MultiSphere MultiSphere;
	};

	// Collision group mask, which categories of objects collide
	// with this rigid body. Derived from RigidBodies.CollisionGroupMask.
	enum class CollisionGroup : std::uint32_t
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

	struct RigidBody
	{
		// Index to the node (bone) to which it is attached.
		std::int16_t NodeIndex = -1;

		// Region/permutation index, for damage states.
		std::int16_t RegionIndex = -1;
		std::int16_t PermutationIndex = -1;

		// Broad-phase: sphere in local-space that encloses all shapes.
		Vec3 BoundingSphereOffset{};
		float BoundingSphereRadius = 0.0f;

		// Collision group mask of the rigid body.
		std::uint32_t CollisionGroupMask = 0;

		// Resolved shapes (may be >1 if RigidBody pointed to a List).
		std::vector<Shape> Shapes;
	};

	struct Phmo
	{
		std::string TagName;

		// Rigid bodies with their shapes already resolved.
		std::vector<RigidBody> RigidBodies;

		// Physical node hierarchy.
		std::vector<Node> Nodes;

		// AABB global derived from all bounding spheres.
		Vec3 BoundsMin{};
		Vec3 BoundsMax{};
	};
}