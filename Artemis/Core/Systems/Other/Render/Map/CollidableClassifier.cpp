#include "pch.h"

#include "CollidableClassifier.h"

#include "Core/Types/Tick/Tick.h"
#include "Core/Types/Environment/Fixtures/Fixtures.h"
#include "Core/Types/Environment/Vitality/ObjectVitality.h"
#include "Core/Types/Egocentric/Affordance/Affordance.h"

namespace
{
    constexpr RgbColor kCollidable = { 0.27f, 0.51f, 0.33f };
    constexpr RgbColor kVitality = { 0.95f, 0.82f, 0.29f };
    constexpr RgbColor kAffordance = { 0.88f, 0.32f, 0.77f };
    constexpr RgbColor kSelected = { 1.0f,  1.0f,  1.0f };

    constexpr RgbColor kObstacle = { 0.27f, 0.51f, 0.33f };
    constexpr RgbColor kTeleporter = { 0.20f, 0.80f, 0.78f };
    constexpr RgbColor kShield = { 0.93f, 0.26f, 0.60f };
    constexpr RgbColor kLift = { 0.18f, 0.35f, 0.95f };
    constexpr RgbColor kDestructible = { 0.93f, 0.35f, 0.13f };

    constexpr RgbColor kTeamRed = { 0.85f, 0.20f, 0.20f };
    constexpr RgbColor kTeamBlue = { 0.22f, 0.40f, 0.90f };
    constexpr RgbColor kTeamGreen = { 0.25f, 0.75f, 0.30f };
    constexpr RgbColor kTeamOrange = { 0.94f, 0.54f, 0.19f };
    constexpr RgbColor kTeamPurple = { 0.60f, 0.30f, 0.85f };
    constexpr RgbColor kTeamGold = { 0.92f, 0.75f, 0.15f };
    constexpr RgbColor kTeamBrown = { 0.55f, 0.36f, 0.20f };
    constexpr RgbColor kTeamPink = { 0.95f, 0.55f, 0.75f };
    constexpr RgbColor kTeamNeutral = { 0.60f, 0.60f, 0.60f };

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