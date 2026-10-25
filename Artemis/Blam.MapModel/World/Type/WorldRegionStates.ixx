export module Resolved.World.Type:RegionStates;

import std;

export namespace Resolved::World::Type::RegionStates
{
    // Region of the render model, which is how the engine indexes the permutations of an object.
    struct EngineRegion
    {
        std::uint32_t Name{};

        // Collision region with the same name, or -1.
        int CollRegion{ -1 };

        // Collision permutation with the same name as each permutation of the region, or -1.
        // note: All -1 when the region has no collision region.
        std::vector<int> CollPermutations{};

        // Meshes the render model draws for each permutation of the region.
        std::vector<int> PermutationMeshCounts{};
    };

    // Which permutation of the render model matches each permutation of the collision model.
    struct RegionStates
    {
        // Regions of the render model, in the order the engine keeps the permutation of each one.
        // note: Empty when the model has no render model.
        std::vector<EngineRegion> EngineRegions{};
    };
}