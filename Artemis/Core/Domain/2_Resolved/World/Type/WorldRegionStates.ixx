export module Resolved.World.Type:RegionStates;

import std;

export namespace Resolved::World::Type::RegionStates
{
    // Permutations of a model by variant.
    struct Variant
    {
        // Permutation of each collision region for each damage state, or -1.
        std::vector<std::array<int, 5>> StateMap{};

        bool HasDestroyedGeometry{};
    };

    // Which permutation of each collision region a model shows, and when it changes.
    struct RegionStates
    {
        std::vector<Variant> Variants{};

        // Damage state of each collision region for each instant response of its damage section.
        std::vector<std::vector<int>> LevelToState{};

        // Damage state each collision region takes when the object dies, or -1.
        std::vector<int> DeathStateMap{};

        // Damage section of each collision region, or -1.
        std::vector<int> RegionToSection{};
    };
}