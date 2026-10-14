module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :FixtureLabels;

import Common.Math.Type;
import Common.Semantic.Type;
import Common.ZoneShape.Type;
import Environment.Fixtures.Type;
import Gui.Format.System;
import Tables.Object.Type;
import Viewer.Options.State;
import Viewer.Options.Type;
import Viewer.Style.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Flag = Viewer::Options::Type::Flag;
    using LabelRow = Viewer::Hud::Type::LabelRow;
    using FixturesFormater = Gui::Format::System::FixturesFormater;
    using TeamFormater = Gui::Format::System::TeamFormater;
    using ZoneShape = Common::ZoneShape::Type::ZoneShape;

    namespace Crate = Tables::Object::Type::Crate;
    namespace Fixture = Environment::Fixtures::Type;
    namespace SemanticType = Common::Semantic::Type;

    using Common::Semantic::Type::Function;

    using Viewer::Style::Type::k_Fixture;
    using Viewer::Style::Type::k_TeamNeutral;
    using Viewer::Style::Type::ColorOfTeam;

    constexpr float k_FixturePriority{ 0.5f };

    constexpr ImU32 k_InfoColor{ IM_COL32(190, 200, 215, 255) };
    constexpr ImU32 k_MutedColor{ IM_COL32(150, 160, 175, 255) };

    auto Row(std::string text, ImU32 color = k_InfoColor) -> LabelRow
    {
        return LabelRow{ std::move(text), color };
    }

    auto ZoneText(const ZoneShape& zone) -> std::string
    {
        if (zone.Kind == Common::ZoneShape::Type::Kind::None) return "zone: none";

        return std::format("zone: {} {:.1f}", FixturesFormater::ZoneKindToString(zone.Kind), zone.Radius);
    }

    auto AllowedText(const Fixture::Teleport::Passes& passes) -> std::string
    {
        std::string text{};
        auto append = [&](const char* name)
        {
            if (!text.empty()) text += ", ";
            text += name;
        };

        if (passes.Players) append("players");
        if (passes.Ground) append("ground");
        if (passes.Heavy) append("heavy");
        if (passes.Flying) append("flying");
        if (passes.Projectiles) append("projectiles");

        return text.empty() ? std::string{ "nothing" } : text;
    }

    auto ShieldKindText(Crate::Shield::Kind kind) -> const char*
    {
        switch (kind)
        {
        case Crate::Shield::Kind::OneWay: return "One-way";
        case Crate::Shield::Kind::TwoWay: return "Two-way";
        case Crate::Shield::Kind::Blocker: return "Blocker";
        }

        return "Unknown";
    }
}

namespace Viewer::Hud::System
{
    auto FixtureLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = context.Options;

        if (!tick.Fixtures) return;

        const auto& fixtures = *tick.Fixtures;

        const ImU32 accent = ToImColor(k_Fixture);
        const ImU32 neutral = ToImColor(k_TeamNeutral);

        auto teamOf = [](const auto& item) -> ImU32
        {
            return ToImColor(ColorOfTeam(item.Team));
        };

        // note: Objective spawns, objective zones and boundaries are entities with a zone detail (D88). The tag name is for debugging.
        auto addZoneEntities = [&](Function function, const char* title, bool isSubtypeShown)
        {
            if (!tick.Entities) return;

            for (const auto& entity : tick.Entities->Rows)
            {
                if (entity.Function != function) continue;

                ZoneShape shape{};
                if (tick.ZoneDetails)
                {
                    const auto zoneIt = tick.ZoneDetails->find(entity.Handle);
                    if (zoneIt != tick.ZoneDetails->end()) shape = zoneIt->second.Shape;
                }

                std::string detail{};
                if (isSubtypeShown)
                {
                    detail = std::string{ SemanticType::Subtype::NameOf(entity.Function, entity.Subtype) };
                }
                else if (tick.ObjectTable)
                {
                    const auto objectIt = tick.ObjectTable->find(entity.Handle);
                    if (objectIt != tick.ObjectTable->end()) detail = ShortTagName(objectIt->second.TagName);
                }

                std::vector<LabelRow> rows{};
                rows.push_back(Row(title, accent));
                rows.push_back(Row(std::move(detail)));
                rows.push_back(Row(std::string{ "team: " } + TeamFormater::TeamToString(entity.Team)));
                rows.push_back(Row(ZoneText(shape), k_MutedColor));

                canvas.Add(entity.WorldOrigin, std::move(rows), accent, teamOf(entity), k_FixturePriority, entity.Handle);
            }
        };

        if (options.IsEnabled(Flag::FixtureTeleportLabels))
        {
            for (const auto& teleport : fixtures.Teleporters)
            {
                std::vector<LabelRow> rows{};
                rows.push_back(Row(std::string{ "TELEPORT " } +
                    FixturesFormater::TeleporterKindToString(teleport.Kind), accent));
                rows.push_back(Row(std::format("channel: {}", teleport.Channel)));
                rows.push_back(Row(ZoneText(teleport.ZoneShape)));
                rows.push_back(Row("allows: " + AllowedText(teleport.Passes), k_MutedColor));

                canvas.Add(teleport.Position, std::move(rows), accent, neutral, k_FixturePriority, teleport.Handle);
            }
        }

        if (options.IsEnabled(Flag::FixtureLiftLabels))
        {
            for (const auto& lift : fixtures.Lifts)
            {
                std::vector<LabelRow> rows{};
                rows.push_back(Row("LIFT", accent));
                rows.push_back(Row(ShortTagName(lift.TagName), k_MutedColor));
                rows.push_back(Row(std::string{ "angle: " } + FixturesFormater::AngleKindToString(lift.Angle)));
                rows.push_back(Row(std::string{ "force: " } + FixturesFormater::ForceKindToString(lift.Force)));

                canvas.Add(lift.Position, std::move(rows), accent, neutral, k_FixturePriority, lift.Handle);
            }
        }

        if (options.IsEnabled(Flag::FixtureShieldLabels))
        {
            for (const auto& shield : fixtures.Shields)
            {
                std::vector<LabelRow> rows{};
                rows.push_back(Row("SHIELD", accent));
                rows.push_back(Row(ShieldKindText(shield.Kind)));

                canvas.Add(shield.Position, std::move(rows), accent, neutral, k_FixturePriority, shield.Handle);
            }
        }

        if (options.IsEnabled(Flag::FixtureObjectives))
        {
            for (const auto& objective : fixtures.Objectives)
            {
                std::vector<LabelRow> rows{};
                rows.push_back(Row("OBJECTIVE", accent));
                rows.push_back(Row(ShortTagName(objective.TagName)));
                rows.push_back(Row(std::string{ "team: " } + TeamFormater::TeamToString(objective.Team)));
                rows.push_back(Row(objective.IsEquipped
                    ? std::format("carrier: 0x{:08X}", objective.CarrierHandle)
                    : std::string{ "carrier: none" }, k_MutedColor));

                canvas.Add(objective.Position, std::move(rows), accent, teamOf(objective), k_FixturePriority, objective.Handle);
            }

            addZoneEntities(Function::ObjectiveSpawn, "OBJECTIVE SPAWN", false);
            addZoneEntities(Function::ObjectiveZone, "OBJECTIVE ZONE", false);
        }

        if (options.IsEnabled(Flag::FixtureDestructibles))
        {
            for (const auto& destructible : fixtures.Destructibles)
            {
                std::vector<LabelRow> rows{};
                rows.push_back(Row(FixturesFormater::DestructibleKindToString(destructible.Kind), accent));
                rows.push_back(Row(std::format("health: {:.2f}", destructible.Health)));

                canvas.Add(destructible.Position, std::move(rows), accent, neutral, k_FixturePriority, destructible.Handle);
            }
        }

        if (options.IsEnabled(Flag::FixtureSpawns))
        {
            for (const auto& spawn : fixtures.Spawns)
            {
                const char* kind = "Initial";
                if (spawn.Kind == Fixture::Spawn::Kind::Respawn) kind = "Respawn";
                else if (spawn.Kind == Fixture::Spawn::Kind::Invisible) kind = "Invisible";

                std::vector<LabelRow> rows{};
                rows.push_back(Row("SPAWN", accent));
                rows.push_back(Row(kind));
                rows.push_back(Row(std::string{ "team: " } + TeamFormater::TeamToString(spawn.Team)));

                canvas.Add(spawn.Position, std::move(rows), accent, teamOf(spawn), k_FixturePriority, spawn.Handle);
            }

            addZoneEntities(Function::Boundary, "BOUNDARY", true);
        }
    }
}