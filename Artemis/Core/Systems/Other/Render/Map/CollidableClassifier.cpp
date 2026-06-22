#include "pch.h"

#include "CollidableClassifier.h"

#include "Core/Types/Tick/Tick.h"
#include "Core/Types/Environment/Fixtures/Fixtures.h"
#include "Core/Types/Environment/Vitality/ObjectVitality.h"
#include "Core/Types/Egocentric/Affordance/Affordance.h"

namespace
{
    constexpr RgbColor kCollidable = { 0.37f, 0.55f, 0.40f };
    constexpr RgbColor kVitality = { 0.84f, 0.70f, 0.36f };
    constexpr RgbColor kAffordance = { 0.66f, 0.44f, 0.72f };
    constexpr RgbColor kSelected = { 1.0f,  1.0f,  1.0f };

    constexpr RgbColor kObstacle = { 0.37f, 0.55f, 0.40f };
    constexpr RgbColor kTeleporter = { 0.32f, 0.66f, 0.62f };
    constexpr RgbColor kShield = { 0.77f, 0.40f, 0.54f };
    constexpr RgbColor kLift = { 0.36f, 0.49f, 0.77f };
    constexpr RgbColor kDestructible = { 0.80f, 0.46f, 0.32f };

    constexpr RgbColor kTeamRed = { 0.77f, 0.31f, 0.31f };
    constexpr RgbColor kTeamBlue = { 0.31f, 0.45f, 0.74f };
    constexpr RgbColor kTeamGreen = { 0.36f, 0.66f, 0.41f };
    constexpr RgbColor kTeamOrange = { 0.83f, 0.56f, 0.28f };
    constexpr RgbColor kTeamPurple = { 0.59f, 0.41f, 0.75f };
    constexpr RgbColor kTeamGold = { 0.84f, 0.71f, 0.35f };
    constexpr RgbColor kTeamBrown = { 0.59f, 0.45f, 0.33f };
    constexpr RgbColor kTeamPink = { 0.81f, 0.55f, 0.67f };
    constexpr RgbColor kTeamNeutral = { 0.55f, 0.55f, 0.55f };

    RgbColor ColorForTeam(Team team)
    {
        switch (team)
        {
        case Team::Red:     return kTeamRed;
        case Team::Blue:    return kTeamBlue;
        case Team::Green:   return kTeamGreen;
        case Team::Orange:  return kTeamOrange;
        case Team::Purple:  return kTeamPurple;
        case Team::Gold:    return kTeamGold;
        case Team::Brown:   return kTeamBrown;
        case Team::Pink:    return kTeamPink;
        case Team::Neutral:
        default:            return kTeamNeutral;
        }
    }
}

void CollidableClassifier::Build(const std::shared_ptr<const Tick>& tick)
{
    m_VitalityHandles.clear();
    m_AffordanceHandles.clear();
    m_FixtureColors.clear();

    if (!tick) return;

    if (tick->Vitalities)
    {
        m_VitalityHandles.reserve(tick->Vitalities->size());
        for (const auto& [handle, vit] : *tick->Vitalities)
            m_VitalityHandles.insert(handle);
    }

    if (tick->Affordances)
    {
        m_AffordanceHandles.reserve(tick->Affordances->size());
        for (const auto& aff : *tick->Affordances)
            m_AffordanceHandles.insert(aff.Handle);
    }

    if (tick->Fixtures)
    {
        const Fixtures& fx = *tick->Fixtures;

        auto addFixed = [&](const auto& vec, RgbColor col) {
            for (const auto& item : vec)
                m_FixtureColors[item.Handle] = col;
            };

        auto addByTeam = [&](const auto& vec) {
            for (const auto& item : vec)
                m_FixtureColors[item.Handle] = ColorForTeam(item.Team);
            };

        addFixed(fx.Obstacles, kObstacle);
        addFixed(fx.Teleporters, kTeleporter);
        addFixed(fx.Lifts, kLift);
        addFixed(fx.Shields, kShield);
        addFixed(fx.Destructibles, kDestructible);

        addByTeam(fx.Spawns);
        addByTeam(fx.ObjectiveSpawns);
        addByTeam(fx.Objectives);
    }
}

RgbColor CollidableClassifier::ColorFor(uint32_t handle) const
{
    if (m_AffordanceHandles.count(handle)) return kAffordance;

    if (auto it = m_FixtureColors.find(handle);
        it != m_FixtureColors.end())
    {
        return it->second;
    }

    if (m_VitalityHandles.count(handle)) return kVitality;

    return kCollidable;
}