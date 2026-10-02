export module Resolved.World.System:Raycast;

import Common.Math.Type;
import Resolved.World.Type;
import Resolved.Definitions.Type;
import Resolved.Definitions.State;
import std;

export namespace Resolved::World::System
{
	class SbspRaycaster
	{
	private:
		using Vec3 = Common::Math::Type::Vec3;
		using Triangle = Common::Math::Type::Triangle;
		using DefinitionsStore = Resolved::Definitions::State::DefinitionsStore;
		using ResolvedSbsp = Resolved::Definitions::Type::Sbsp::Sbsp;
		using Hit = Resolved::World::Type::Raycast::Hit;
		using CellKey = Resolved::World::Type::Raycast::CellKey;
		using CellKeyHash = Resolved::World::Type::Raycast::CellKeyHash;
		using TriangleRef = Resolved::World::Type::Raycast::TriangleRef;

	public:
		SbspRaycaster() = default;
		~SbspRaycaster() = default;

		auto Cast(const DefinitionsStore& definitionsStore, const Vec3& origin,
			const Vec3& direction, float maxDistance) -> Hit;

		auto Cleanup() -> void;

	private:
		bool m_GridBuilt{ false };
		std::vector<const ResolvedSbsp*> m_Sbsps{};
		std::unordered_map<CellKey, std::vector<TriangleRef>, CellKeyHash> m_Cells{};

		auto EnsureGrid(const DefinitionsStore& definitionsStore) -> void;
		auto CellKeyFor(const Vec3& point) const -> CellKey;
	};
}