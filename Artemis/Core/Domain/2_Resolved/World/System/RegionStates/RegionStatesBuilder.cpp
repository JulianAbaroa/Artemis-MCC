module Resolved.World.System;
import :RegionStates;

namespace Resolved::World::System
{
    auto RegionStatesBuilder::Build(const Coll& coll, const Mode* mode) -> RegionStates
    {
        RegionStates out{};

        if (mode) out.EngineRegions = this->BuildEngineRegions(coll, *mode);

        return out;
    }

    auto RegionStatesBuilder::BuildEngineRegions(const Coll& coll, const Mode& mode) -> std::vector<EngineRegion>
    {
        std::vector<EngineRegion> regions{};
        regions.reserve(mode.Regions.size());

        for (const auto& modeRegion : mode.Regions)
        {
            EngineRegion entry{};

            entry.Name = modeRegion.NameId;
            entry.PermutationMeshCounts = modeRegion.PermutationMeshCounts;
            entry.CollPermutations.assign(modeRegion.PermutationNames.size(), -1);

            for (std::size_t region = 0; region < coll.RegionNames.size(); ++region)
            {
                if (coll.RegionNames[region] == modeRegion.NameId)
                {
                    entry.CollRegion = static_cast<int>(region);
                    break;
                }
            }

            if (entry.CollRegion >= 0)
            {
                const auto& collPermutations = coll.PermutationNames[entry.CollRegion];

                for (std::size_t permutation = 0;
                    permutation < modeRegion.PermutationNames.size();
                    ++permutation)
                {
                    for (std::size_t collPermutation = 0;
                        collPermutation < collPermutations.size();
                        ++collPermutation)
                    {
                        if (collPermutations[collPermutation] == modeRegion.PermutationNames[permutation])
                        {
                            entry.CollPermutations[permutation] = static_cast<int>(collPermutation);
                            break;
                        }
                    }
                }
            }

            regions.push_back(std::move(entry));
        }

        return regions;
    }
}