module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :Fixtures;

import Common.Semantic.Type;
import Common.ZoneShape.Type;
import Environment.Details.Type;
import Gui.Widget.System;
import Gui.Format.System;
import std;

namespace
{
    using Tick = Export::Tick::Type::Tick;
    using Export::Tick::Type::Entity;
    using Export::Tick::Type::ZoneDetail;
    using Export::Tick::Type::TeleporterDetail;
    using Export::Tick::Type::LiftDetail;
    using Export::Tick::Type::BarrierDetail;
    using Environment::Details::Type::TeleporterPass;
    using Environment::Details::Type::HasPass;
    using ZoneShape = Common::ZoneShape::Type::ZoneShape;
    using ZoneKind = Common::ZoneShape::Type::Kind;

    using HexFormater = Gui::Format::System::HexFormater;
    using TeamFormater = Gui::Format::System::TeamFormater;
    using FixturesFormater = Gui::Format::System::FixturesFormater;

    using PanelGuiService = Gui::Widget::System::PanelGuiService;

    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Binding;

    namespace SemanticType = Common::Semantic::Type;

    auto DrawZoneShape(const ZoneShape& zoneShape) -> void
    {
        ImGui::TextDisabled("Zone Shape:");
        ImGui::Text("%s", FixturesFormater::ZoneKindToString(zoneShape.Kind));

        switch (zoneShape.Kind)
        {
        case ZoneKind::Cylinder:
            ImGui::Text("Radius: %.2f", zoneShape.Radius);
            ImGui::Text("Top: %.2f", zoneShape.Top);
            ImGui::Text("Bottom: %.2f", zoneShape.Bottom);
            break;

        case ZoneKind::Box:
            ImGui::Text("Width: %.2f", zoneShape.Radius);
            ImGui::Text("Length: %.2f", zoneShape.Length);
            ImGui::Text("Top: %.2f", zoneShape.Top);
            ImGui::Text("Bottom: %.2f", zoneShape.Bottom);
            break;

        default:
            break;
        }
    }

    auto DrawAllowedObjects(TeleporterPass passes) -> void
    {
        struct Entry { bool IsActive; const char* Label; };

        const Entry entries[] = {
            { HasPass(passes, TeleporterPass::Players),        "Players" },
            { HasPass(passes, TeleporterPass::GroundVehicles), "Ground Vehicles" },
            { HasPass(passes, TeleporterPass::HeavyVehicles),  "Heavy Vehicles" },
            { HasPass(passes, TeleporterPass::FlyingVehicles), "Flying Vehicles" },
            { HasPass(passes, TeleporterPass::Projectiles),    "Projectiles" },
        };

        for (const Entry& entry : entries)
        {
            ImGui::TextColored(entry.IsActive ?
                ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                "%s %s", entry.IsActive ? "[+]" : "[-]", entry.Label);
        }
    }

    // Draws an entity with a teleporter detail (D90).
    // param shape: Empty when the entity has no zone detail.
    auto DrawTeleporter(const Entity& entity, const TeleporterDetail& teleporter, const ZoneShape& shape,
        const std::string& tagName) -> void
    {
        const std::string_view subtype{ SemanticType::Subtype::NameOf(entity.Function, entity.Subtype) };

        PanelGuiService::DrawHeader(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Teleporter", tagName, entity.Handle);
        PanelGuiService::DrawVec3("World Origin:", entity.WorldOrigin);
        PanelGuiService::DrawVec3("Forward:", entity.Forward);
        PanelGuiService::DrawVec3("Up:", entity.Up);
        ImGui::Text("Channel: %d", static_cast<int>(teleporter.Channel));
        ImGui::Text("Type: %.*s", static_cast<int>(subtype.size()), subtype.data());
        ImGui::Text("Destinations: %d", static_cast<int>(teleporter.Destinations.size()));

        for (const std::uint32_t destination : teleporter.Destinations)
        {
            ImGui::BulletText("%s", HexFormater::Hex32(destination).c_str());
        }

        ImGui::Spacing();
        DrawZoneShape(shape);

        ImGui::Spacing();
        ImGui::TextDisabled("Allowed Objects:");
        DrawAllowedObjects(teleporter.Passes);
    }

    // return: The title of an entity: function and subtype.
    auto TitleOf(const Entity& entity) -> std::string
    {
        std::string title{ SemanticType::NameOf(entity.Function) };
        const std::string_view subtype{ SemanticType::Subtype::NameOf(entity.Function, entity.Subtype) };
        if (!subtype.empty()) title += std::format(" {}", subtype);

        return title;
    }

    // Draws an entity with a lift detail (D91).
    // note: The force waits for A12 and the launch direction is provisional.
    auto DrawLift(const Entity& entity, const LiftDetail& lift, const std::string& tagName) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), TitleOf(entity).c_str(), tagName, entity.Handle);
        PanelGuiService::DrawVec3("Bounding Center:", entity.BoundingCenter);
        PanelGuiService::DrawVec3("Forward:", entity.Forward);
        PanelGuiService::DrawVec3("Up:", entity.Up);
        PanelGuiService::DrawVec3("Launch Direction:", lift.LaunchDirection);
    }

    // Draws an entity with a barrier detail (D91).
    auto DrawBarrier(const Entity& entity, const BarrierDetail& barrier, const std::string& tagName) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.85f, 0.4f, 1.0f, 1.0f), TitleOf(entity).c_str(), tagName, entity.Handle);
        PanelGuiService::DrawVec3("Bounding Center:", entity.BoundingCenter);
        PanelGuiService::DrawVec3("Forward:", entity.Forward);
        PanelGuiService::DrawVec3("Up:", entity.Up);
        PanelGuiService::DrawVec3("Pass Direction:", barrier.PassDirection);
    }

    // Draws an entity with a zone detail: objective spawn, objective zone, boundary or any other zone (D88).
    // param shape: Empty when the entity has no zone detail.
    auto DrawZoneEntity(const Entity& entity, const ZoneShape& shape, const std::string& tagName) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), TitleOf(entity).c_str(), tagName, entity.Handle);
        PanelGuiService::DrawVec3("World Origin:", entity.WorldOrigin);
        PanelGuiService::DrawVec3("Forward:", entity.Forward);
        PanelGuiService::DrawVec3("Up:", entity.Up);
        ImGui::Text("Team: %s", TeamFormater::TeamToString(entity.Team));

        ImGui::Spacing();
        DrawZoneShape(shape);
    }

    // Draws a prop entity: a physical object without a game role (D92).
    auto DrawProp(const Entity& entity, const std::string& tagName) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(1.0f, 0.55f, 0.0f, 1.0f), TitleOf(entity).c_str(), tagName, entity.Handle);
        PanelGuiService::DrawVec3("Bounding Center:", entity.BoundingCenter);
        PanelGuiService::DrawVec3("Forward:", entity.Forward);
        PanelGuiService::DrawVec3("Up:", entity.Up);
        PanelGuiService::DrawVec3("Linear Velocity:", entity.LinearVelocity);
        PanelGuiService::DrawVec3("Angular Velocity:", entity.AngularVelocity);
        ImGui::Text("Radius: %.2f", entity.BoundingRadius);
    }

    // Draws a spawn point entity (D92). The kind is the subtype.
    auto DrawSpawnPoint(const Entity& entity, const std::string& tagName) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), TitleOf(entity).c_str(), tagName, entity.Handle);
        PanelGuiService::DrawVec3("Bounding Center:", entity.BoundingCenter);
        PanelGuiService::DrawVec3("Forward:", entity.Forward);
        ImGui::Text("Team: %s", TeamFormater::TeamToString(entity.Team));
    }

    // Draws an objective item entity: flag, bomb or ball (D92).
    // note: Carried is the binding of the entity and the carrier is its BoundTo.
    auto DrawObjectiveItem(const Entity& entity, const std::string& tagName) -> void
    {
        const bool isCarried{ entity.Binding == Binding::Carried };
        const std::string title{ TitleOf(entity) + (isCarried ? " (Carried)" : "") };

        PanelGuiService::DrawHeader(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), title.c_str(), tagName, entity.Handle);
        PanelGuiService::DrawVec3("Bounding Center:", entity.BoundingCenter);
        PanelGuiService::DrawVec3("Forward:", entity.Forward);
        PanelGuiService::DrawVec3("Up:", entity.Up);
        PanelGuiService::DrawVec3("Linear Velocity:", entity.LinearVelocity);
        PanelGuiService::DrawVec3("Angular Velocity:", entity.AngularVelocity);
        ImGui::Text("Team: %s", TeamFormater::TeamToString(entity.Team));

        if (isCarried)
        {
            ImGui::Text("Carrier Handle: %s", HexFormater::Hex32(entity.BoundTo).c_str());
        }
    }
}

namespace Viewer::Hud::System
{
    auto FixturesHud::Draw(const Tick& tick, std::uint32_t handle) -> void
    {
        // note: Every fixture is an entity, with its details when it has them (D88, D90, D91, D92).
        const Entity* entity{ tick.Entities ? tick.Entities->Find(handle) : nullptr };
        if (entity == nullptr)
        {
            ImGui::TextDisabled("Selected object is not a live entity.");
            return;
        }

        const ZoneDetail* zone{ nullptr };
        if (tick.ZoneDetails)
        {
            const auto zoneIt = tick.ZoneDetails->find(handle);
            if (zoneIt != tick.ZoneDetails->end()) zone = &zoneIt->second;
        }

        std::string tagName{};
        if (tick.ObjectTable)
        {
            const auto objectIt = tick.ObjectTable->find(handle);
            if (objectIt != tick.ObjectTable->end()) tagName = objectIt->second.TagName;
        }

        const ZoneShape shape{ zone != nullptr ? zone->Shape : ZoneShape{} };

        if (entity->Function == Function::Teleporter && tick.TeleporterDetails)
        {
            const auto teleporterIt = tick.TeleporterDetails->find(handle);
            if (teleporterIt != tick.TeleporterDetails->end())
            {
                DrawTeleporter(*entity, teleporterIt->second, shape, tagName);
                return;
            }
        }

        if (entity->Function == Function::Lift && tick.LiftDetails)
        {
            const auto liftIt = tick.LiftDetails->find(handle);
            if (liftIt != tick.LiftDetails->end())
            {
                DrawLift(*entity, liftIt->second, tagName);
                return;
            }
        }

        if (entity->Function == Function::Barrier && tick.BarrierDetails)
        {
            const auto barrierIt = tick.BarrierDetails->find(handle);
            if (barrierIt != tick.BarrierDetails->end())
            {
                DrawBarrier(*entity, barrierIt->second, tagName);
                return;
            }
        }

        const bool isZoneFunction{ entity->Function == Function::ObjectiveSpawn || entity->Function == Function::ObjectiveZone
            || entity->Function == Function::Boundary };

        if (zone != nullptr || isZoneFunction)
        {
            DrawZoneEntity(*entity, shape, tagName);
            return;
        }

        switch (entity->Function)
        {
        case Function::Prop:
            DrawProp(*entity, tagName);
            return;

        case Function::SpawnPoint:
            DrawSpawnPoint(*entity, tagName);
            return;

        case Function::ObjectiveItem:
            DrawObjectiveItem(*entity, tagName);
            return;

        default:
            break;
        }

        ImGui::TextDisabled("Selected object is not a fixture.");
    }
}