export module Resolved.World.System:RegionStates;

import Resolved.Definitions.Type;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
    // Matches, for a model, the regions and permutations of the render model with the collision ones.
    // note: The engine keeps the permutation of each region of the render model in memory, so nothing is inferred from the damage.
    class RegionStatesBuilder
    {
    private:
        using Coll = Resolved::Definitions::Type::Coll::Coll;
        using Mode = Resolved::Definitions::Type::Mode::Mode;
        using RegionStates = Resolved::World::Type::RegionStates::RegionStates;
        using EngineRegion = Resolved::World::Type::RegionStates::EngineRegion;

    public:
        RegionStatesBuilder() = default;
        ~RegionStatesBuilder() = default;

        // param mode: Render model, which gives the regions and permutations the engine indexes. May be null.
        // note: Without it the engine permutations cannot be matched, so the result has no regions.
        auto Build(const Coll& coll, const Mode* mode) -> RegionStates;

    private:
        // Matches the regions and permutations of the render model with the collision ones by name.
        auto BuildEngineRegions(const Coll& coll, const Mode& mode) -> std::vector<EngineRegion>;
    };
}