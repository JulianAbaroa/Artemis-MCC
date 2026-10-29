export module Blam.MapModel.World.System:RegionStates;

import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.Type;
import std;

export namespace Blam::MapModel::World::System
{
    // Matches, for a model, the regions and permutations of the render model with the collision ones.
    // note: The engine keeps the permutation of each region of the render model in memory, so nothing is inferred from the damage.
    class RegionStatesBuilder
    {
    private:
        using Coll = Blam::MapModel::Definitions::Type::Coll::Coll;
        using Mode = Blam::MapModel::Definitions::Type::Mode::Mode;
        using RegionStates = Blam::MapModel::World::Type::RegionStates::RegionStates;
        using EngineRegion = Blam::MapModel::World::Type::RegionStates::EngineRegion;

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