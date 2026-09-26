export module Resolved.World.System:Raycast;

import Common.Math.Type;
import Resolved.World.Type;
import Resolved.World.State;
import std;

export namespace Resolved::World::System
{
	class SbspRaycaster
	{
	private:
		using Vec3 = Common::Math::Type::Vec3;
		using Triangle = Common::Math::Type::Triangle;
		using WorldStore = Resolved::World::State::WorldStore;
		using Hit = Resolved::World::Type::Raycast::Hit;
		using CellKey = Resolved::World::Type::Raycast::CellKey;
		using CellKeyHash = Resolved::World::Type::Raycast::CellKeyHash;
		using TriangleRef = Resolved::World::Type::Raycast::TriangleRef;

	public:
		SbspRaycaster() = default;
		~SbspRaycaster() = default;

		auto Cast(const WorldStore& worldStore, const Vec3& origin,
			const Vec3& direction, float maxDistance) -> Hit;

		auto Cleanup() -> void;

	private:
		bool m_GridBuilt{ false };
		std::unordered_map<CellKey, std::vector<TriangleRef>, CellKeyHash> m_Cells{};

		auto EnsureGrid(const WorldStore& worldStore) -> void;
		auto CellKeyFor(const Vec3& point) const -> CellKey;
	};
}