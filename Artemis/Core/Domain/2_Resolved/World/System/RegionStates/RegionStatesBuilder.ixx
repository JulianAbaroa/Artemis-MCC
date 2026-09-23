export module Resolved.World.System:RegionStates;

import Map.Tag.Type;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
	class RegionStatesBuilder
	{
	private:
		using HlmtObject = Map::Tag::Type::Hlmt::Object::HlmtObject;
		using Hlmt_VariantsObject = Map::Tag::Type::Hlmt::Object::Hlmt_VariantsObject;
		using ResolvedColl = Resolved::World::Type::Coll::Coll;
		using ResolvedRegionStates = Resolved::World::Type::RegionStates::RegionStates;
		using RegionStatesVariant = Resolved::World::Type::RegionStates::Variant;
		using StateMap = std::vector<std::array<int, 5>>;
		using LevelToState = std::vector<std::vector<int>>;

	public:
		RegionStatesBuilder() = default;
		~RegionStatesBuilder() = default;

		auto Build(const HlmtObject& hlmt, const ResolvedColl& coll) -> ResolvedRegionStates;

	private:
		auto BuildStateMap(const Hlmt_VariantsObject& variant,
			const ResolvedColl& coll) -> StateMap;

		auto BuildLevelToState(const HlmtObject& hlmt,
			const ResolvedColl& coll, LevelToState& levelToState) -> void;

		auto BuildDeathStateMap(const HlmtObject& hlmt,
			const ResolvedColl& coll, std::vector<int>& deathState) -> void;

		auto BuildRegionToSection(const HlmtObject& hlmt,
			const ResolvedColl& coll) -> std::vector<int>;
	};
}