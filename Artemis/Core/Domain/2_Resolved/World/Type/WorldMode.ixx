export module Resolved.World.Type:Mode;

import Common.Math.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
	using Vec4 = Common::Math::Type::Vec4;
	using Node = Common::Math::Type::Node;
}

export namespace Resolved::World::Type::Mode
{
	struct Marker
	{
		std::int8_t NodeIndex{ -1 };
		std::uint8_t Flags{ 0 };

		Vec3 Translation{};
		Vec4 Rotation{};
		Vec3 Direction{};
		float Scale{ 1.0f };
	};

	struct MarkerGroup
	{
		std::uint32_t NameId;
		std::vector<Marker> Markers;
	};

	struct Bounds
	{
		Vec3 Min{};
		Vec3 Max{};
	};

	struct Mode
	{
		std::string TagName;
		std::vector<MarkerGroup> MarkerGroups;
		std::vector<Node> Nodes;
		Bounds ModelBounds{};
	};
}