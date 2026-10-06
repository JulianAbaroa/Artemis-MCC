module Viewer.Palette.System;

import Viewer.Style.Type;
import std;

// TODO: Draw respawn/safe/death zones.

namespace
{
    using Viewer::Style::Type::k_Health;
    using Viewer::Style::Type::k_Obstacle;
    using Viewer::Style::Type::k_Teleporter;
    using Viewer::Style::Type::k_Lift;
    using Viewer::Style::Type::k_Shield;
    using Viewer::Style::Type::k_Destructible;
    using Viewer::Style::Type::k_Affordance;
    using Viewer::Style::Type::k_Collidable;
}

namespace Viewer::Palette::System
{
    auto PaletteService::Build(const std::shared_ptr<const Tick>& tick) -> void
    {
        if (!tick)
        {
            this->Reset();
            return;
        }

        if (m_IsGenerationSet && tick->Generation == m_LastGeneration) return;

        m_LastGeneration = tick->Generation;
        m_IsGenerationSet = true;

        m_Entries.clear();

        std::uint32_t rank{ 0 };
        auto add = [this, &rank](std::uint32_t handle, const Color& color)
        {
            m_Entries.push_back(Entry{ handle, rank++, color });
        };

        if (tick->Healths)
        {
            for (const auto& entry : *tick->Healths) add(entry.first, k_Health);
        }

        if (tick->Fixtures)
        {
            const auto& fixtures = *tick->Fixtures;

            auto addFixed = [&add](const auto& items, const Color& color)
            {
                for (const auto& item : items) add(item.Handle, color);
            };

            auto addByTeam = [&add](const auto& items)
            {
                for (const auto& item : items) add(item.Handle, Viewer::Style::Type::ColorOfTeam(item.Team));
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

        if (tick->Affordances)
        {
            for (const auto& affordance : *tick->Affordances) add(affordance.Handle, k_Affordance);
        }

        std::sort(m_Entries.begin(), m_Entries.end(), [](const Entry& a, const Entry& b) {
            if (a.Handle != b.Handle) return a.Handle < b.Handle;
            return a.Rank > b.Rank;
        });
    }

    auto PaletteService::Reset() -> void
    {
        m_Entries.clear();

        m_LastGeneration = 0;
        m_IsGenerationSet = false;
    }

    auto PaletteService::ColorOf(std::uint32_t handle) const -> Color
    {
        const auto it = std::lower_bound(m_Entries.begin(), m_Entries.end(), handle,
            [](const Entry& entry, std::uint32_t value) { return entry.Handle < value; });

        if (it != m_Entries.end() && it->Handle == handle) return it->Value;

        return k_Collidable;
    }
}