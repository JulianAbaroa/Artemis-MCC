module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :FixtureLabels;

import Common.Math.Type;
import Common.Semantic.Type;
import Common.ZoneShape.Type;
import Perception.Details.Type;
import DevTools.Gui.Format.System;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Style.Type;
import DevTools.Viewer.Hud.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Hud::Type::LabelRow;
    using DevTools::Gui::Format::System::FixturesFormater;
    using DevTools::Gui::Format::System::TeamFormater;
    using Common::ZoneShape::Type::ZoneShape;

    namespace SemanticType = Common::Semantic::Type;

    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Binding;
    using Perception::Details::Type::TeleporterPass;
    using Perception::Details::Type::HasPass;

    using DevTools::Viewer::Style::Type::k_Fixture;
    using DevTools::Viewer::Style::Type::k_TeamNeutral;
    using DevTools::Viewer::Style::Type::ColorOfTeam;

    using DevTools::Viewer::Hud::Type::k_InfoColor;
    using DevTools::Viewer::Hud::Type::k_MutedColor;

    constexpr float k_FixturePriority{ 0.5f };

    auto Row(std::string text, ImU32 color = k_InfoColor) -> LabelRow
    {
        return LabelRow{ std::move(text), color };
    }

    auto ZoneText(const ZoneShape& zone) -> std::string
    {
        if (zone.Kind == Common::ZoneShape::Type::ZoneShapeKind::None) return "zone: none";

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

namespace DevTools::Viewer::Hud::System
{
    auto FixtureLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = canvas.GetOptions();

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
            // note: Objective items are entities (D92). Carried is the binding and the carrier is BoundTo.
            if (tick.Entities)
            {
                for (const auto& entity : tick.Entities->Rows)
                {
                    if (entity.Function != Function::ObjectiveItem) continue;

                    const bool isCarried{ entity.Binding == Binding::Carried };

                    std::vector<LabelRow> rows{};
                    rows.push_back(Row(std::format("OBJECTIVE {}", SemanticType::Subtype::NameOf(entity.Function, entity.Subtype)), accent));
                    rows.push_back(Row(tagNameOf(entity.Handle)));
                    rows.push_back(Row(std::string{ "team: " } + TeamFormater::TeamToString(entity.Team)));
                    rows.push_back(Row(isCarried ? std::format("carrier: 0x{:08X}", entity.BoundTo) : std::string{ "carrier: none" }, k_MutedColor));

                    canvas.Add(entity.BoundingCenter, std::move(rows), accent, teamOf(entity), k_FixturePriority, entity.Handle);
                }
            }

            addZoneEntities(Function::ObjectiveSpawn, "OBJECTIVE SPAWN", false);
            addZoneEntities(Function::ObjectiveZone, "OBJECTIVE ZONE", false);
        }

        if (options.IsEnabled(Flag::FixtureSpawns))
        {
            // note: Spawn points are entities (D92). The kind (Initial, Respawn, Invisible) is the subtype.
            if (tick.Entities)
            {
                for (const auto& entity : tick.Entities->Rows)
                {
                    if (entity.Function != Function::SpawnPoint) continue;

                    std::vector<LabelRow> rows{};
                    rows.push_back(Row("SPAWN", accent));
                    rows.push_back(Row(std::string{ SemanticType::Subtype::NameOf(entity.Function, entity.Subtype) }));
                    rows.push_back(Row(std::string{ "team: " } + TeamFormater::TeamToString(entity.Team)));

                    canvas.Add(entity.BoundingCenter, std::move(rows), accent, teamOf(entity), k_FixturePriority, entity.Handle);
                }
            }

            addZoneEntities(Function::Boundary, "BOUNDARY", true);
        }
    }
}