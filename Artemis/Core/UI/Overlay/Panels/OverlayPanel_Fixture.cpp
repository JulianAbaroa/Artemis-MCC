#include "pch.h"

#include "OverlayPanel_Fixture.h"

#include "Core/Types/Tick/Tick.h"
#include "Core/Types/Environment/Fixtures/Fixtures.h"

#include "Core/UI/Utils/Hex/HexFormater.h"
#include "Core/UI/Utils/String/EnumToString.h"

#include "External/imgui/imgui.h"

namespace
{
    void DrawZoneShape(ZoneShape zoneShape)
    {
        ImGui::TextDisabled("Zone Shape:");
        switch (zoneShape.ShapeType)
        {
        case ShapeType::None:
            ImGui::Text("None");
            break;
        case ShapeType::Cylinder:
            ImGui::Text("Cylinder");
            ImGui::Text("Radius: %.2f", zoneShape.Radius);
            ImGui::Text("Top: %.2f", zoneShape.Top);
            ImGui::Text("Bottom: %.2f", zoneShape.Bottom);
            break;
        case ShapeType::Box:
            ImGui::Text("Box");
            ImGui::Text("Width: %.2f", zoneShape.Radius);
            ImGui::Text("Length: %.2f", zoneShape.Length);
            ImGui::Text("Top: %.2f", zoneShape.Top);
            ImGui::Text("Bottom: %.2f", zoneShape.Bottom);
            break;
        }
    }

    void DrawAllowedObjects(AllowedObjects allowed)
    {
        uint8_t raw = static_cast<uint8_t>(allowed);

        struct Flag { uint8_t bit; const char* label; bool inverted; };
        constexpr Flag flags[] = {
            { 0x01, "Players",         true  },
            { 0x02, "Ground Vehicles", false },
            { 0x04, "Heavy Vehicles",  false },
            { 0x08, "Flying Vehicles", false },
            { 0x10, "Projectiles",     false },
        };

        for (const auto& flag : flags)
        {
            bool bitSet = (raw & flag.bit) != 0;
            bool active = flag.inverted ? !bitSet : bitSet;
            ImGui::TextColored(active ?
                ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 0.4f, 0.4f, 1.0f),
                "%s %s", active ? "[+]" : "[-]", flag.label);
        }
    }

    void DrawObstacle(const Obstacle& o)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.0f, 1.0f), "Obstacle");
        ImGui::Separator();
        ImGui::Text("%s", o.TagName.c_str());
        ImGui::Text("%s", HexFormater::Hex32(o.Handle).c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", o.Position[0], o.Position[1], o.Position[2]);
        ImGui::Text("Forward: %.2f, %.2f, %.2f", o.Forward[0], o.Forward[1], o.Forward[2]);
        ImGui::Text("Up: %.2f, %.2f, %.2f", o.Up[0], o.Up[1], o.Up[2]);
        ImGui::Text("Linear Velocity: %.2f, %.2f, %.2f", o.LinearVelocity[0], o.LinearVelocity[1], o.LinearVelocity[2]);
        ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f", o.AngularVelocity[0], o.AngularVelocity[1], o.AngularVelocity[2]);
        ImGui::Text("Radius: %.2f", o.BoundingRadius);
    }

    void DrawSpawn(const Spawn& s)
    {
        const char* typeStr = s.Type == SpawnType::Initial ? "Initial Spawn"
            : s.Type == SpawnType::Respawn ? "Respawn" : "Invisible";
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", typeStr);
        ImGui::Separator();
        ImGui::Text("%s", s.TagName.c_str());
        ImGui::Text("%s", HexFormater::Hex32(s.Handle).c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", s.Position[0], s.Position[1], s.Position[2]);
        ImGui::Text("Forward: %.2f, %.2f, %.2f", s.Forward[0], s.Forward[1], s.Forward[2]);
        ImGui::Text("Team: %s", EnumToString::TeamToString(s.Team));
    }

    void DrawTeleporter(const Teleporter& t)
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Teleporter");
        ImGui::Separator();
        ImGui::Text("%s", t.TagName.c_str());
        ImGui::Text("%s", HexFormater::Hex32(t.Handle).c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", t.Position[0], t.Position[1], t.Position[2]);
        ImGui::Text("Forward: %.2f, %.2f, %.2f", t.Forward[0], t.Forward[1], t.Forward[2]);
        ImGui::Text("Up: %.2f, %.2f, %.2f", t.Up[0], t.Up[1], t.Up[2]);
        ImGui::Text("Channel: %d", static_cast<int>(t.Channel));
        ImGui::Text("Type: %s", EnumToString::TeleporterTypeToString(t.Type));
        ImGui::Spacing();
        DrawZoneShape(t.ZoneShape);
        ImGui::Spacing();
        ImGui::TextDisabled("Allowed Objects:");
        DrawAllowedObjects(t.AllowedObjects);
    }

    void DrawLift(const Lift& l)
    {
        ImGui::TextColored(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), "Lift");
        ImGui::Separator();
        ImGui::Text("%s", l.TagName.c_str());
        ImGui::Text("%s", HexFormater::Hex32(l.Handle).c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", l.Position[0], l.Position[1], l.Position[2]);
        ImGui::Text("Forward: %.2f, %.2f, %.2f", l.Forward[0], l.Forward[1], l.Forward[2]);
        ImGui::Text("Up: %.2f, %.2f, %.2f", l.Up[0], l.Up[1], l.Up[2]);
        ImGui::Text("Angle Type: %s", EnumToString::AngleTypeToString(l.AngleType));
        ImGui::Text("Force Type: %s", EnumToString::ForceTypeToString(l.ForceType));
        ImGui::Text("Launch Direction: %.2f, %.2f, %.2f", l.LaunchDirection[0], l.LaunchDirection[1], l.LaunchDirection[2]);
    }

    void DrawShield(const Shield& sh)
    {
        const char* typeStr = sh.Type == ShieldType::OneWay ? "One-Way"
            : sh.Type == ShieldType::TwoWay ? "Two-Way" : "Blocker";
        ImGui::TextColored(ImVec4(0.85f, 0.4f, 1.0f, 1.0f), "Shield");
        ImGui::Separator();
        ImGui::Text("%s", sh.TagName.c_str());
        ImGui::Text("%s", HexFormater::Hex32(sh.Handle).c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", sh.Position[0], sh.Position[1], sh.Position[2]);
        ImGui::Text("Forward: %.2f, %.2f, %.2f", sh.Forward[0], sh.Forward[1], sh.Forward[2]);
        ImGui::Text("Up: %.2f, %.2f, %.2f", sh.Up[0], sh.Up[1], sh.Up[2]);
        ImGui::Text("Type: %s", typeStr);
        if (sh.BlockDirection.has_value())
            ImGui::Text("Block Direction: %.2f, %.2f, %.2f",
                (*sh.BlockDirection)[0], (*sh.BlockDirection)[1], (*sh.BlockDirection)[2]);
    }

    void DrawObjectiveSpawn(const ObjectiveSpawn& os)
    {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Objective Spawn");
        ImGui::Separator();
        ImGui::Text("%s", os.TagName.c_str());
        ImGui::Text("%s", HexFormater::Hex32(os.Handle).c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", os.Position[0], os.Position[1], os.Position[2]);
        ImGui::Text("Forward: %.2f, %.2f, %.2f", os.Forward[0], os.Forward[1], os.Forward[2]);
        ImGui::Text("Up: %.2f, %.2f, %.2f", os.Up[0], os.Up[1], os.Up[2]);
        ImGui::Text("Team: %s", EnumToString::TeamToString(os.Team));
        ImGui::Spacing();
        DrawZoneShape(os.ZoneShape);
    }

    void DrawObjective(const Objective& ob)
    {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s",
            ob.IsEquipped ? "Objective (Carried)" : "Objective");
        ImGui::Separator();
        ImGui::Text("%s", ob.TagName.c_str());
        ImGui::Text("%s", HexFormater::Hex32(ob.Handle).c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", ob.Position[0], ob.Position[1], ob.Position[2]);
        ImGui::Text("Forward: %.2f, %.2f, %.2f", ob.Forward[0], ob.Forward[1], ob.Forward[2]);
        ImGui::Text("Up: %.2f, %.2f, %.2f", ob.Up[0], ob.Up[1], ob.Up[2]);
        ImGui::Text("Linear Velocity: %.2f, %.2f, %.2f", ob.LinearVelocity[0], ob.LinearVelocity[1], ob.LinearVelocity[2]);
        ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f", ob.AngularVelocity[0], ob.AngularVelocity[1], ob.AngularVelocity[2]);
        ImGui::Text("Team: %s", EnumToString::TeamToString(ob.Team));
        ImGui::Text("Is Equipped: %s", ob.IsEquipped ? "Yes" : "No");
        if (ob.IsEquipped)
            ImGui::Text("Carrier Handle: %s", HexFormater::Hex32(ob.CarrierHandle).c_str());
    }

    void DrawDestructible(const Destructible& d)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Destructible");
        ImGui::Separator();
        ImGui::Text("%s", d.TagName.c_str());
        ImGui::Text("%s", HexFormater::Hex32(d.Handle).c_str());
        ImGui::Text("Position: %.2f, %.2f, %.2f", d.Position[0], d.Position[1], d.Position[2]);
        ImGui::Text("Forward: %.2f, %.2f, %.2f", d.Forward[0], d.Forward[1], d.Forward[2]);
        ImGui::Text("Up: %.2f, %.2f, %.2f", d.Up[0], d.Up[1], d.Up[2]);
        ImGui::Text("Linear Velocity: %.2f, %.2f, %.2f", d.LinearVelocity[0], d.LinearVelocity[1], d.LinearVelocity[2]);
        ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f", d.AngularVelocity[0], d.AngularVelocity[1], d.AngularVelocity[2]);
        ImGui::Text("Type: %s", EnumToString::DestructibleTypeToString(d.Type));
        ImGui::Text("Health: %.2f", d.Health);
    }
}

void OverlayPanel_Fixture::Draw(
    const std::shared_ptr<const Tick>& tick, uint32_t handle)
{
    if (!tick || !tick->Fixtures)
    {
        ImGui::TextDisabled("No fixture data.");
        return;
    }

    const Fixtures& fx = *tick->Fixtures;

    for (const auto& o : fx.Obstacles)       if (o.Handle == handle) { DrawObstacle(o);       return; }
    for (const auto& s : fx.Spawns)          if (s.Handle == handle) { DrawSpawn(s);          return; }
    for (const auto& t : fx.Teleporters)     if (t.Handle == handle) { DrawTeleporter(t);     return; }
    for (const auto& l : fx.Lifts)           if (l.Handle == handle) { DrawLift(l);           return; }
    for (const auto& sh : fx.Shields)        if (sh.Handle == handle) { DrawShield(sh);        return; }
    for (const auto& os : fx.ObjectiveSpawns)if (os.Handle == handle) { DrawObjectiveSpawn(os);return; }
    for (const auto& ob : fx.Objectives)     if (ob.Handle == handle) { DrawObjective(ob);     return; }
    for (const auto& d : fx.Destructibles)   if (d.Handle == handle) { DrawDestructible(d);   return; }

    ImGui::TextDisabled("Selected object is not a fixture.");
}