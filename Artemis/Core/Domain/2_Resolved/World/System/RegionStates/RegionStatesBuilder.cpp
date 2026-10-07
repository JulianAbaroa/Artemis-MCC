module Resolved.World.System;
import :RegionStates;

namespace
{
    using Resolved::World::Type::RegionStates::Variant;
}

namespace Resolved::World::System
{
    auto RegionStatesBuilder::Build(const Hlmt& hlmt, const Coll& coll) -> RegionStates
    {
        RegionStates out{};

        out.Variants.reserve(hlmt.Variants.size());

        for (const auto& hlmtVariant : hlmt.Variants)
        {
            Variant entry{};

            entry.StateMap = this->BuildStateMap(hlmtVariant, coll);

            for (const auto& region : entry.StateMap)
            {
                if (region[4] >= 0)
                {
                    entry.HasDestroyedGeometry = true;
                    break;
                }
            }

            out.Variants.push_back(std::move(entry));
        }

        this->BuildLevelToState(hlmt, coll, out.LevelToState);
        this->BuildDeathStateMap(hlmt, coll, out.DeathStateMap);
        out.RegionToSection = this->BuildRegionToSection(hlmt, coll);

        return out;
    }

    auto RegionStatesBuilder::BuildStateMap(const Resolved::Definitions::Type::Hlmt::Variant& variant,
        const Coll& coll) -> std::vector<std::array<int, 5>>
    {
        std::vector<std::array<int, 5>> stateMap(
            coll.RegionNames.size(),
            std::array<int, 5>{ -1, -1, -1, -1, -1 });

        for (std::size_t variantRegion = 0;
            variantRegion < variant.Regions.size();
            ++variantRegion)
        {
            const auto& hlmtRegion = variant.Regions[variantRegion];

            int collRegionIdx{ -1 };
            for (std::size_t regionName = 0;
                regionName < coll.RegionNames.size();
                ++regionName)
            {
                if (coll.RegionNames[regionName] == hlmtRegion.RegionName)
                {
                    collRegionIdx = static_cast<int>(regionName);
                    break;
                }
            }

            if (collRegionIdx < 0) continue;

            for (const auto& permutation : hlmtRegion.Permutations)
            {
                for (const auto& state : permutation.States)
                {
                    if (state.State > 4) continue;

                    const std::uint32_t name = state.PermutationName;
                    if (name == 0) continue;

                    const auto& names =
                        coll.PermutationNames[collRegionIdx];

                    for (std::size_t current = 0;
                        current < names.size();
                        ++current)
                    {
                        if (names[current] == name)
                        {
                            stateMap[collRegionIdx][state.State] = static_cast<int>(current);
                            break;
                        }
                    }
                }
            }
        }

        return stateMap;
    }

    auto RegionStatesBuilder::BuildLevelToState(const Hlmt& hlmt, const Coll& coll,
        std::vector<std::vector<int>>& levelToState) -> void
    {
        levelToState.assign(coll.RegionNames.size(), {});

        for (const auto& damageSection : hlmt.DamageSections)
        {
            int ownRegion{ -1 };

            for (std::size_t current = 0;
                current < coll.RegionNames.size();
                ++current)
            {
                if (coll.RegionNames[current] == damageSection.Name)
                {
                    ownRegion = static_cast<int>(current);
                    break;
                }
            }

            if (ownRegion < 0) continue;

            auto& levels = levelToState[ownRegion];
            levels.resize(damageSection.InstantResponses.size(), -1);

            for (std::size_t instantResponse = 0;
                instantResponse < damageSection.InstantResponses.size();
                ++instantResponse)
            {
                for (const auto& regionTransition :
                    damageSection.InstantResponses[
                        instantResponse].RegionTransitions)
                {
                    if (regionTransition.Region ==
                        damageSection.Name)
                    {
                        levels[instantResponse] =
                            static_cast<int>(regionTransition.NewState);

                        break;
                    }
                }
            }
        }
    }

    auto RegionStatesBuilder::BuildDeathStateMap(const Hlmt& hlmt, const Coll& coll,
        std::vector<int>& deathState) -> void
    {
        deathState.assign(coll.RegionNames.size(), -1);

        for (const auto& damageSection : hlmt.DamageSections)
        {
            bool isOwnedRegionSection{};

            for (std::size_t current = 0;
                current < coll.RegionNames.size();
                ++current)
            {
                if (coll.RegionNames[current] == damageSection.Name)
                {
                    isOwnedRegionSection = true;
                    break;
                }
            }

            if (isOwnedRegionSection) continue;

            for (const auto& instantResponse :
                damageSection.InstantResponses)
            {
                if (instantResponse.DamageThreshold > 0.0001f) continue;

                for (const auto& regionTransition :
                    instantResponse.RegionTransitions)
                {
                    for (std::size_t current = 0;
                        current < coll.RegionNames.size();
                        ++current)
                    {
                        if (coll.RegionNames[current] ==
                            regionTransition.Region)
                        {
                            if (static_cast<int>(regionTransition.NewState) >
                                deathState[current])
                            {
                                deathState[current] =
                                    static_cast<int>(regionTransition.NewState);
                            }

                            break;
                        }
                    }
                }
            }
        }
    }

    auto RegionStatesBuilder::BuildRegionToSection(const Hlmt& hlmt, const Coll& coll) -> std::vector<int>
    {
        std::vector<int> regionToSection(coll.RegionNames.size(), -1);

        for (std::size_t regionName = 0;
            regionName < coll.RegionNames.size();
            ++regionName)
        {
            for (std::size_t damageSection = 0;
                damageSection < hlmt.DamageSections.size();
                ++damageSection)
            {
                if (coll.RegionNames[regionName] ==
                    hlmt.DamageSections[damageSection].Name)
                {
                    regionToSection[regionName] = static_cast<int>(damageSection);
                    break;
                }
            }
        }

        return regionToSection;
    }
}