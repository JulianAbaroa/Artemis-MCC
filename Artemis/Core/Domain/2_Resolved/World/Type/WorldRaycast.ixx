export module Resolved.World.Type:Raycast;

import Common.Math.Type;
import std;

namespace
{
	using Vec3 = Common::Math::Type::Vec3;
}

export namespace Resolved::World::Type::Raycast
{
	struct CellKey
	{
		std::int32_t X{};
		std::int32_t Y{};
		std::int32_t Z{};

		auto operator==(const CellKey&) const -> bool = default;
	};

	struct CellKeyHash
	{
		auto operator()(const CellKey& key) const -> std::size_t
		{
			std::size_t h = static_cast<std::size_t>(static_cast<std::uint32_t>(key.X));
			h = h * 73856093u ^ static_cast<std::size_t>(static_cast<std::uint32_t>(key.Y)) * 19349663u;
			h = h ^ static_cast<std::size_t>(static_cast<std::uint32_t>(key.Z)) * 83492791u;
			return h;
		}
	};

	struct TriangleRef
	{
		std::int32_t SbspIndex{};
		std::int32_t TriangleIndex{};
	};

	struct Hit
	{
		bool IsHit{ false };
		float Distance{ 0.0f };
		Vec3 Point{};
	};
}