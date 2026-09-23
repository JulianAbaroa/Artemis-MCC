export module Resolved.World.Type:Coll;

import Common.Math.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using Node = Common::Math::Type::Node;
	using Triangle = Common::Math::Type::Triangle;
}

export namespace Resolved::World::Type::Coll
{
	struct Mesh
	{
		std::int16_t NodeIndex = -1;

		std::int16_t RegionIndex = -1;
		std::int16_t PermutationIndex = -1;

		std::vector<Triangle> Triangles;

		Vec3 LocalMin = {};
		Vec3 LocalMax = {};
	};

	struct Coll
	{
		std::string TagName;

		std::vector<Node> Nodes;
		std::vector<Mesh> Meshes;

		std::vector<std::uint32_t> RegionNames;
		std::vector<std::vector<std::uint32_t>> PermutationNames;
		std::vector<int> DefaultPermutationIndex;

		Vec3 BoundsMin = {};
		Vec3 BoundsMax = {};
	};
}