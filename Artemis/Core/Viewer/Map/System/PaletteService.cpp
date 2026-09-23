module Viewer.Map.System;
import :Palette;

import std;

// TODO: Draw shape types transparent (teleports, objectives, zones).
// TODO: Draw respawn/safe/death zones.

namespace
{
	using Color = Viewer::Map::Type::Color;
	using Team = Common::Team::Type::Team;

	constexpr Color k_Collidable = { 0.37f, 0.55f, 0.40f };
	constexpr Color k_Health = { 0.84f, 0.70f, 0.36f };
	constexpr Color k_Affordance = { 0.66f, 0.44f, 0.72f };

	constexpr Color k_Obstacle = { 0.37f, 0.55f, 0.40f };
	constexpr Color k_Teleporter = { 0.32f, 0.66f, 0.62f };
	constexpr Color k_Shield = { 0.77f, 0.40f, 0.54f };
	constexpr Color k_Lift = { 0.36f, 0.49f, 0.77f };
	constexpr Color k_Destructible = { 0.80f, 0.46f, 0.32f };

	constexpr Color k_TeamRed = { 0.77f, 0.31f, 0.31f };
	constexpr Color k_TeamBlue = { 0.31f, 0.45f, 0.74f };
	constexpr Color k_TeamGreen = { 0.36f, 0.66f, 0.41f };
	constexpr Color k_TeamOrange = { 0.83f, 0.56f, 0.28f };
	constexpr Color k_TeamPurple = { 0.59f, 0.41f, 0.75f };
	constexpr Color k_TeamGold = { 0.84f, 0.71f, 0.35f };
	constexpr Color k_TeamBrown = { 0.59f, 0.45f, 0.33f };
	constexpr Color k_TeamPink = { 0.81f, 0.55f, 0.67f };
	constexpr Color k_TeamNeutral = { 0.55f, 0.55f, 0.55f };

	auto ColorOfTeam(Team team) -> Color
	{
		switch (team)
		{
		case Team::Red:     return k_TeamRed;
		case Team::Blue:    return k_TeamBlue;
		case Team::Green:   return k_TeamGreen;
		case Team::Orange:  return k_TeamOrange;
		case Team::Purple:  return k_TeamPurple;
		case Team::Gold:    return k_TeamGold;
		case Team::Brown:   return k_TeamBrown;
		case Team::Pink:    return k_TeamPink;
		case Team::Neutral:
		default:            return k_TeamNeutral;
		}
	}
}

namespace Viewer::Map::System
{
	auto PaletteService::Build(const std::shared_ptr<const Tick>& tick) -> void
	{
		if (!tick)
		{
			this->Reset();
			return;
		}

		if (m_HasGeneration && tick->Generation == m_LastGeneration) return;

		m_LastGeneration = tick->Generation;
		m_HasGeneration = true;

		m_HealthHandles.clear();
		m_AffordanceHandles.clear();
		m_FixtureColors.clear();

		if (tick->Healths)
		{
			m_HealthHandles.reserve(tick->Healths->size());
			for (const auto& entry : *tick->Healths)
			{
				m_HealthHandles.insert(entry.first);
			}
		}

		if (tick->Affordances)
		{
			m_AffordanceHandles.reserve(tick->Affordances->size());
			for (const auto& affordance : *tick->Affordances)
			{
				m_AffordanceHandles.insert(affordance.Handle);
			}
		}

		if (tick->Fixtures)
		{
			const auto& fixtures = *tick->Fixtures;

			auto addFixed = [this](const auto& items, Color color) {
				for (const auto& item : items) m_FixtureColors[item.Handle] = color;
				};

			auto addByTeam = [this](const auto& items) {
				for (const auto& item : items) m_FixtureColors[item.Handle] = ColorOfTeam(item.Team);
				};

			addFixed(fixtures.Obstacles, k_Obstacle);
			addFixed(fixtures.Teleporters, k_Teleporter);
			addFixed(fixtures.Lifts, k_Lift);
			addFixed(fixtures.Shields, k_Shield);
			addFixed(fixtures.Destructibles, k_Destructible);

			addByTeam(fixtures.Spawns);
			addByTeam(fixtures.ObjectiveSpawns);
			addByTeam(fixtures.Objectives);
		}
	}

	auto PaletteService::Reset() -> void
	{
		m_HealthHandles.clear();
		m_AffordanceHandles.clear();
		m_FixtureColors.clear();

		m_LastGeneration = 0;
		m_HasGeneration = false;
	}

	auto PaletteService::ColorOf(std::uint32_t handle) const -> Color
	{
		if (m_AffordanceHandles.contains(handle)) return k_Affordance;

		if (const auto it = m_FixtureColors.find(handle); it != m_FixtureColors.end())
		{
			return it->second;
		}

		if (m_HealthHandles.contains(handle)) return k_Health;

		return k_Collidable;
	}
}