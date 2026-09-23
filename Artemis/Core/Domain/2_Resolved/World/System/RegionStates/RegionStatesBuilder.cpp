module Resolved.World.System;
import :RegionStates;

namespace Resolved::World::System
{
	auto RegionStatesBuilder::Build(const HlmtObject& hlmt,
		const ResolvedColl& coll) -> ResolvedRegionStates
	{
		ResolvedRegionStates out;

		out.Variants.reserve(hlmt.Variants.size());

		for (const Hlmt_VariantsObject& variant : hlmt.Variants)
		{
			RegionStatesVariant entry;
			entry.StateMap = this->BuildStateMap(variant, coll);

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

	auto RegionStatesBuilder::BuildStateMap(const Hlmt_VariantsObject& variant,
		const ResolvedColl& coll) -> StateMap
	{
		std::vector<std::array<int, 5>> stateMap(
			coll.RegionNames.size(),
			std::array<int, 5>{ -1, -1, -1, -1, -1 });

		for (size_t variantRegion = 0;
			variantRegion < variant.Regions.size();
			++variantRegion)
		{
			const auto& hlmtRegion = variant.Regions[variantRegion];

			int collRegionIdx = -1;
			for (size_t regionName = 0;
				regionName < coll.RegionNames.size();
				++regionName)
			{
				if (coll.RegionNames[regionName] == hlmtRegion.RegionName)
				{
					collRegionIdx = (int)regionName;
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

					for (size_t current = 0;
						current < names.size();
						++current)
					{
						if (names[current] == name)
						{
							stateMap[collRegionIdx][state.State] = (int)current;
							break;
						}
					}
				}
			}
		}

		return stateMap;
	}

	auto RegionStatesBuilder::BuildLevelToState(const HlmtObject& hlmt,
		const ResolvedColl& coll, LevelToState& levelToState) -> void
	{
		levelToState.assign(coll.RegionNames.size(), {});

		for (const auto& damageSection : hlmt.DamageSections)
		{
			int ownRegion = -1;

			for (size_t current = 0;
				current < coll.RegionNames.size();
				++current)
			{
				if (coll.RegionNames[current] == damageSection.Name)
				{
					ownRegion = (int)current;
					break;
				}
			}

			if (ownRegion < 0) continue;

			auto& levels = levelToState[ownRegion];
			levels.resize(damageSection.InstantResponses.size(), -1);

			for (size_t instantResponse = 0;
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
							(int)regionTransition.NewState;

						break;
					}
				}
			}
		}
	}

	auto RegionStatesBuilder::BuildDeathStateMap(const HlmtObject& hlmt,
		const ResolvedColl& coll, std::vector<int>& deathState) -> void
	{
		deathState.assign(coll.RegionNames.size(), -1);

		for (const auto& damageSection : hlmt.DamageSections)
		{
			bool isOwnedRegionSection = false;

			for (size_t current = 0;
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
					for (size_t current = 0;
						current < coll.RegionNames.size();
						++current)
					{
						if (coll.RegionNames[current] ==
							regionTransition.Region)
						{
							if ((int)regionTransition.NewState >
								deathState[current])
							{
								deathState[current] =
									(int)regionTransition.NewState;
							}

							break;
						}
					}
				}
			}
		}
	}

	auto RegionStatesBuilder::BuildRegionToSection(const HlmtObject& hlmt,
		const ResolvedColl& coll) -> std::vector<int>
	{
		std::vector<int> map(coll.RegionNames.size(), -1);

		for (size_t regionName = 0;
			regionName < coll.RegionNames.size();
			++regionName)
		{
			for (size_t damageSection = 0;
				damageSection < hlmt.DamageSections.size();
				++damageSection)
			{
				if (coll.RegionNames[regionName] ==
					hlmt.DamageSections[damageSection].Name)
				{
					map[regionName] = (int)damageSection;
					break;
				}
			}
		}

		return map;
	}
}