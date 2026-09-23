export module Viewer.Map.Type;

import Common.Math.Type;
import std;

export namespace Viewer::Map::Type
{
	using Vec3 = Common::Math::Type::Vec3;

	struct Color
	{
		float R{}, G{}, B{};
	};

	struct ObjectBounds
	{
		std::uint32_t Handle{};

		Vec3 Min{};
		Vec3 Max{};
	};
}