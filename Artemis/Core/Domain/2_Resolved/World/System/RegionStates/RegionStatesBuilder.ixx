export module Resolved.World.System:RegionStates;

import Resolved.Definitions.Type;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
    // Builds, for a model, the permutation each collision region shows in every damage state, and when it changes.
    // note: Reverse engineered from the behavior of the damage sections in memory, so it is not 1:1 with the game yet.
    class RegionStatesBuilder
    {
    private:
        using Hlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using Coll = Resolved::Definitions::Type::Coll::Coll;
        using RegionStates = Resolved::World::Type::RegionStates::RegionStates;

    public:
        RegionStatesBuilder() = default;
        ~RegionStatesBuilder() = default;

        auto Build(const Hlmt& hlmt, const Coll& coll) -> RegionStates;

    private:
        // return: Permutation of each collision region for each damage state, or -1.
        auto BuildStateMap(const Resolved::Definitions::Type::Hlmt::Variant& variant,
            const Coll& coll) -> std::vector<std::array<int, 5>>;

        // Reads the damage state each instant response of a region damage section moves its own region to.
        auto BuildLevelToState(const Hlmt& hlmt, const Coll& coll,
            std::vector<std::vector<int>>& levelToState) -> void;

        // Reads the damage state each region takes when the object dies.
        // note: Only the instant responses of sections that do not own a region count, and only those with no threshold.
        auto BuildDeathStateMap(const Hlmt& hlmt, const Coll& coll,
            std::vector<int>& deathState) -> void;

        // return: Damage section of each collision region, or -1.
        auto BuildRegionToSection(const Hlmt& hlmt, const Coll& coll) -> std::vector<int>;
    };
}