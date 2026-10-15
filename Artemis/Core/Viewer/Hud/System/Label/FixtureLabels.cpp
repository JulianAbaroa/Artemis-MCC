module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :FixtureLabels;

import Common.Math.Type;
import Common.Semantic.Type;
import Common.ZoneShape.Type;
import Environment.Fixtures.Type;
import Environment.Details.Type;
import Gui.Format.System;
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

    namespace Fixture = Environment::Fixtures::Type;
    namespace SemanticType = Common::Semantic::Type;

    using Common::Semantic::Type::Function;
    using Environment::Details::Type::TeleporterPass;
    using Environment::Details::Type::HasPass;

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

    auto AllowedText(TeleporterPass passes) -> std::string
    {
        std::string text{};
        auto append = [&](const char* name)
        {
            if (!text.empty()) text += ", ";
            text += name;
        };

        if (HasPass(passes, TeleporterPass::Players)) append("players");
        if (HasPass(passes, TeleporterPass::GroundVehicles)) append("ground");
        if (HasPass(passes, TeleporterPass::HeavyVehicles)) append("heavy");
        if (HasPass(passes, TeleporterPass::FlyingVehicles)) append("flying");
        if (HasPass(passes, TeleporterPass::Projectiles)) append("projectiles");

        return text.empty() ? std::string{ "nothing" } : text;
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

        // note: Teleporters are entities with a teleporter detail and a zone detail (D90).
        if (options.IsEnabled(Flag::FixtureTeleportLabels) && tick.TeleporterDetails && tick.Entities)
        {
            for (const auto& [handle, teleporter] : *tick.TeleporterDetails)
            {
                const auto* entity{ tick.Entities->Find(handle) };
                if (!entity) continue;

                ZoneShape shape{};
                if (tick.ZoneDetails)
                {
                    const auto zoneIt = tick.ZoneDetails->find(handle);
                    if (zoneIt != tick.ZoneDetails->end()) shape = zoneIt->second.Shape;
                }

                std::vector<LabelRow> rows{};
                rows.push_back(Row(std::format("TELEPORT {}", SemanticType::Subtype::NameOf(entity->Function, entity->Subtype)), accent));
                rows.push_back(Row(std::format("channel: {}", teleporter.Channel)));
                rows.push_back(Row(ZoneText(shape)));
                rows.push_back(Row("allows: " + AllowedText(teleporter.Passes), k_MutedColor));

                canvas.Add(entity->WorldOrigin, std::move(rows), accent, neutral, k_FixturePriority, handle);
            }
        }

        // note: Lifts and barriers are entities with their detail (D91). The kind is the subtype. The force of a lift waits for A12.
        auto tagNameOf = [&tick](std::uint32_t handle) -> std::string
        {
            if (!tick.ObjectTable) return {};

            const auto objectIt = tick.ObjectTable->find(handle);
            return objectIt != tick.ObjectTable->end() ? ShortTagName(objectIt->second.TagName) : std::string{};
        };

        if (options.IsEnabled(Flag::FixtureLiftLabels) && tick.LiftDetails && tick.Entities)
        {
            for (const auto& [handle, lift] : *tick.LiftDetails)
            {
                const auto* entity{ tick.Entities->Find(handle) };
                if (!entity) continue;

                std::vector<LabelRow> rows{};
                rows.push_back(Row(std::format("LIFT {}", SemanticType::Subtype::NameOf(entity->Function, entity->Subtype)), accent));
                rows.push_back(Row(tagNameOf(handle), k_MutedColor));

                canvas.Add(entity->BoundingCenter, std::move(rows), accent, neutral, k_FixturePriority, handle);
            }
        }

        if (options.IsEnabled(Flag::FixtureShieldLabels) && tick.BarrierDetails && tick.Entities)
        {
            for (const auto& [handle, barrier] : *tick.BarrierDetails)
            {
                const auto* entity{ tick.Entities->Find(handle) };
                if (!entity) continue;

                std::vector<LabelRow> rows{};
                rows.push_back(Row(std::format("BARRIER {}", SemanticType::Subtype::NameOf(entity->Function, entity->Subtype)), accent));
                rows.push_back(Row(tagNameOf(handle), k_MutedColor));

                canvas.Add(entity->BoundingCenter, std::move(rows), accent, neutral, k_FixturePriority, handle);
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