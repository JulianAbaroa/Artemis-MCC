export module Resolved.World.Type:RegionStates;

import std;

export namespace Resolved::World::Type::RegionStates
{
	struct Variant
	{
		std::vector<std::array<int, 5>> StateMap{};
		bool HasDestroyedGeometry{ false };
	};

	struct RegionStates
	{
		std::vector<Variant> Variants;
		std::vector<std::vector<int>> LevelToState;
		std::vector<int> DeathStateMap;
		std::vector<int> RegionToSection;
	};
}