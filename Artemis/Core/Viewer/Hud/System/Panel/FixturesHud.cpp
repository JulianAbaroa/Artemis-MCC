module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :Fixtures;

import Common.ZoneShape.Type;
import Tables.Object.Type;
import Environment.Fixtures.Type;
import Gui.Widget.System;
import Gui.Format.System;
import std;

namespace
{
    namespace Fixture = Environment::Fixtures::Type;

    using Tick = Export::Tick::Type::Tick;
    using Fixtures = Export::Tick::Type::Fixtures;
    using ZoneShape = Common::ZoneShape::Type::ZoneShape;
    using ZoneKind = Common::ZoneShape::Type::Kind;
    using ShieldKind = Tables::Object::Type::Crate::Shield::Kind;
    using SpawnKind = Fixture::Spawn::Kind;

    using HexFormater = Gui::Format::System::HexFormater;
    using TeamFormater = Gui::Format::System::TeamFormater;
    using FixturesFormater = Gui::Format::System::FixturesFormater;

    using PanelGuiService = Gui::Widget::System::PanelGuiService;

    auto SpawnKindToString(SpawnKind kind) -> const char*
    {
        switch (kind)
        {
        case SpawnKind::Initial: return "Initial Spawn";
        case SpawnKind::Respawn: return "Respawn";
        case SpawnKind::Invisible: return "Invisible";
        default: return "Unknown";
        }
    }

    auto ShieldKindToString(ShieldKind kind) -> const char*
    {
        switch (kind)
        {
        case ShieldKind::OneWay: return "One-Way";
        case ShieldKind::TwoWay: return "Two-Way";
        case ShieldKind::Blocker: return "Blocker";
        default: return "Unknown";
        }
    }

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

    auto DrawAllowedObjects(const Fixture::Teleport::Passes& passes) -> void
    {
        struct Entry { bool IsActive; const char* Label; };

        const Entry entries[] = {
            { passes.Players,     "Players" },
            { passes.Ground,      "Ground Vehicles" },
            { passes.Heavy,       "Heavy Vehicles" },
            { passes.Flying,      "Flying Vehicles" },
            { passes.Projectiles, "Projectiles" },
        };

        for (const Entry& entry : entries)
        {
            ImGui::TextColored(entry.IsActive ?
                ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                "%s %s", entry.IsActive ? "[+]" : "[-]", entry.Label);
        }
    }

    auto DrawObstacle(const Fixture::Obstacle::Obstacle& o) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(1.0f, 0.55f, 0.0f, 1.0f), "Obstacle", o.TagName, o.Handle);
        PanelGuiService::DrawVec3("Position:", o.Position);
        PanelGuiService::DrawVec3("Forward:", o.Forward);
        PanelGuiService::DrawVec3("Up:", o.Up);
        PanelGuiService::DrawVec3("Linear Velocity:", o.LinearVelocity);
        PanelGuiService::DrawVec3("Angular Velocity:", o.AngularVelocity);
        ImGui::Text("Radius: %.2f", o.BoundingRadius);
    }

    auto DrawSpawn(const Fixture::Spawn::Spawn& s) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), SpawnKindToString(s.Kind), s.TagName, s.Handle);
        PanelGuiService::DrawVec3("Position:", s.Position);
        PanelGuiService::DrawVec3("Forward:", s.Forward);
        ImGui::Text("Team: %s", TeamFormater::TeamToString(s.Team));
    }

    auto DrawTeleporter(const Fixture::Teleport::Teleport& t) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Teleporter", t.TagName, t.Handle);
        PanelGuiService::DrawVec3("Position:", t.Position);
        PanelGuiService::DrawVec3("Forward:", t.Forward);
        PanelGuiService::DrawVec3("Up:", t.Up);
        ImGui::Text("Channel: %d", static_cast<int>(t.Channel));
        ImGui::Text("Type: %s", FixturesFormater::TeleporterKindToString(t.Kind));
        ImGui::Text("Destinations: %d", static_cast<int>(t.DestinationPositions.size()));

        ImGui::Spacing();
        DrawZoneShape(t.ZoneShape);

        ImGui::Spacing();
        ImGui::TextDisabled("Allowed Objects:");
        DrawAllowedObjects(t.Passes);
    }

    auto DrawLift(const Fixture::Lift::Lift& l) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), "Lift", l.TagName, l.Handle);
        PanelGuiService::DrawVec3("Position:", l.Position);
        PanelGuiService::DrawVec3("Forward:", l.Forward);
        PanelGuiService::DrawVec3("Up:", l.Up);
        ImGui::Text("Angle Type: %s", FixturesFormater::AngleKindToString(l.Angle));
        ImGui::Text("Force Type: %s", FixturesFormater::ForceKindToString(l.Force));
        PanelGuiService::DrawVec3("Launch Direction:", l.LaunchDirection);
    }

    auto DrawShield(const Fixture::Shield::Shield& sh) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.85f, 0.4f, 1.0f, 1.0f), "Shield", sh.TagName, sh.Handle);
        PanelGuiService::DrawVec3("Position:", sh.Position);
        PanelGuiService::DrawVec3("Forward:", sh.Forward);
        PanelGuiService::DrawVec3("Up:", sh.Up);
        ImGui::Text("Type: %s", ShieldKindToString(sh.Kind));

        if (sh.BlockDirection.has_value())
        {
            PanelGuiService::DrawVec3("Block Direction:", *sh.BlockDirection);
        }
    }

    auto DrawObjectiveSpawn(const Fixture::ObjectiveSpawn::ObjectiveSpawn& os) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Objective Spawn", os.TagName, os.Handle);
        PanelGuiService::DrawVec3("Position:", os.Position);
        PanelGuiService::DrawVec3("Forward:", os.Forward);
        PanelGuiService::DrawVec3("Up:", os.Up);
        ImGui::Text("Team: %s", TeamFormater::TeamToString(os.Team));

        ImGui::Spacing();
        DrawZoneShape(os.ZoneShape);
    }

    auto DrawObjective(const Fixture::Objective::Objective& ob) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
            ob.IsEquipped ? "Objective (Carried)" : "Objective", ob.TagName, ob.Handle);
        PanelGuiService::DrawVec3("Position:", ob.Position);
        PanelGuiService::DrawVec3("Forward:", ob.Forward);
        PanelGuiService::DrawVec3("Up:", ob.Up);
        PanelGuiService::DrawVec3("Linear Velocity:", ob.LinearVelocity);
        PanelGuiService::DrawVec3("Angular Velocity:", ob.AngularVelocity);
        ImGui::Text("Team: %s", TeamFormater::TeamToString(ob.Team));
        ImGui::Text("Is Equipped: %s", ob.IsEquipped ? "Yes" : "No");

        if (ob.IsEquipped)
        {
            ImGui::Text("Carrier Handle: %s", HexFormater::Hex32(ob.CarrierHandle).c_str());
        }
    }

    auto DrawDestructible(const Fixture::Destructible::Destructible& d) -> void
    {
        PanelGuiService::DrawHeader(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Destructible", d.TagName, d.Handle);
        PanelGuiService::DrawVec3("Position:", d.Position);
        PanelGuiService::DrawVec3("Forward:", d.Forward);
        PanelGuiService::DrawVec3("Up:", d.Up);
        PanelGuiService::DrawVec3("Linear Velocity:", d.LinearVelocity);
        PanelGuiService::DrawVec3("Angular Velocity:", d.AngularVelocity);
        ImGui::Text("Type: %s", FixturesFormater::DestructibleKindToString(d.Kind));
        ImGui::Text("Health: %.2f", d.Health);
    }

    template <typename T, typename Drawer>
    auto TryDraw(const std::vector<T>& items, std::uint32_t handle, Drawer drawer) -> bool
    {
        for (const T& item : items)
        {
            if (item.Handle != handle) continue;

            drawer(item);
            return true;
        }

        return false;
    }
}

namespace Viewer::Hud::System
{
    auto FixturesHud::Draw(const Tick& tick, std::uint32_t handle) -> void
    {
        if (!tick.Fixtures)
        {
            ImGui::TextDisabled("No fixture data.");
            return;
        }

        const Fixtures& fx = *tick.Fixtures;

        if (TryDraw(fx.Obstacles, handle, DrawObstacle)) return;
        if (TryDraw(fx.Spawns, handle, DrawSpawn)) return;
        if (TryDraw(fx.Teleporters, handle, DrawTeleporter)) return;
        if (TryDraw(fx.Lifts, handle, DrawLift)) return;
        if (TryDraw(fx.Shields, handle, DrawShield)) return;
        if (TryDraw(fx.ObjectiveSpawns, handle, DrawObjectiveSpawn)) return;
        if (TryDraw(fx.Objectives, handle, DrawObjective)) return;
        if (TryDraw(fx.Destructibles, handle, DrawDestructible)) return;

        ImGui::TextDisabled("Selected object is not a fixture.");
    }
}