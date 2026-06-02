#include "pch.h"

#include "../UI_Navigation.h"

#include "Core/UI/Utils/Hex/HexFormater.h"

void UI_Navigation::DrawSelectionCluster(
    const AINavigationCluster& navCluster) const
{
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Cluster");

    ImGui::Separator();

    ImGui::Text("Index: %d  (SBSP %d)", navCluster.ClusterIndex,
        navCluster.SbspIndex);

    ImGui::Text("Center: %.1f, %.1f, %.1f", navCluster.Center[0],
        navCluster.Center[1], navCluster.Center[2]);

    ImGui::Text("Size: %.1f x %.1f x %.1f", navCluster.BoundsMax[0] -
        navCluster.BoundsMin[0], navCluster.BoundsMax[1] -
        navCluster.BoundsMin[1], navCluster.BoundsMax[2] -
        navCluster.BoundsMin[2]);

    ImGui::Text("Links:  %d", static_cast<int>(navCluster.Links.size()));

    ImGui::Spacing();

    ImGui::TextDisabled("Neighbors:");

    for (const auto& link : navCluster.Links)
    {
        bool isCross = link.SbspIndex != navCluster.SbspIndex;

        ImGui::TextColored(isCross ?
            ImVec4(1.0f, 0.8f, 0.4f, 1.0f) : ImVec4(0.6f, 1.0f, 0.6f, 1.0f),
            " [%s] SBSP %d -> Cluster %d  (r=%.1f)", isCross ?
            "SEAM  " : "PORTAL", link.SbspIndex, link.ClusterIndex,
            link.PassRadius);
    }
}

void UI_Navigation::DrawSelectionObstacle(const ActiveObstacle& obstacle) const
{
    ImGui::TextColored(ImVec4(1.0f, 0.55f, 0.0f, 1.0f), "Obstacle");

    ImGui::Separator();

    ImGui::Text("%s", obstacle.TagName.c_str());

    ImGui::Text("%s", HexFormater::Hex32(obstacle.Handle).c_str());

    ImGui::Text("Position: %.2f, %.2f, %.2f", obstacle.Position[0],
        obstacle.Position[1], obstacle.Position[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", obstacle.Forward[0],
        obstacle.Forward[1], obstacle.Forward[2]);

    ImGui::Text("Up: %.2f, %.2f, %.2f", obstacle.Up[0], obstacle.Up[1],
        obstacle.Up[2]);

    ImGui::Text("Linear Velocity: %.2f, %.2f, %.2f",
        obstacle.LinearVelocity[0], obstacle.LinearVelocity[1],
        obstacle.LinearVelocity[2]);

    ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f",
        obstacle.AngularVelocity[0], obstacle.AngularVelocity[1],
        obstacle.AngularVelocity[2]);

    ImGui::Text("Radius: %.2f", obstacle.BoundingRadius);
}

void UI_Navigation::DrawSelectionSpawn(const ActiveSpawn& spawn) const
{
    const char* typeStr = spawn.Type == SpawnType::Initial ?
        "Initial Spawn" : spawn.Type == SpawnType::Respawn ?
        "Respawn" : "Invisible";

    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", typeStr);

    ImGui::Separator();

    ImGui::Text("%s", spawn.TagName.c_str());

    ImGui::Text("%s", HexFormater::Hex32(spawn.Handle).c_str());

    ImGui::Text("Position: %.2f, %.2f, %.2f", spawn.Position[0],
        spawn.Position[1], spawn.Position[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", spawn.Forward[0],
        spawn.Forward[1], spawn.Forward[2]);

    ImGui::Text("Team: %s", EnumToString::TeamToString(spawn.Team));
}

void UI_Navigation::DrawSelectionTeleporter(const ActiveTeleporter& teleporter) const
{
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "Teleporter");

    ImGui::Separator();

    ImGui::Text("%s", teleporter.TagName.c_str());

    ImGui::Text("%s", HexFormater::Hex32(teleporter.Handle).c_str());

    ImGui::Text("Position: %.2f, %.2f, %.2f", teleporter.Position[0],
        teleporter.Position[1], teleporter.Position[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", teleporter.Forward[0],
        teleporter.Forward[1], teleporter.Forward[2]);

    ImGui::Text("Up: %.2f, %.2f, %.2f", teleporter.Up[0],
        teleporter.Up[1], teleporter.Up[2]);

    ImGui::Text("Channel: %d", static_cast<int>(teleporter.Channel));

    ImGui::Text("Type: %s", EnumToString::TeleporterTypeToString(
        teleporter.Type));

    ImGui::Spacing();

    this->DrawZoneShape(teleporter.ZoneShape);

    ImGui::Spacing();

    ImGui::TextDisabled("Allowed Objects:");

    this->DrawTeleporterAllowedObjects(teleporter.AllowedObjects);
}

void UI_Navigation::DrawTeleporterAllowedObjects(
    AllowedObjects allowed) const
{
    uint8_t raw = static_cast<uint8_t>(allowed);

    struct Flag
    {
        uint8_t bit;
        const char* label;
        bool inverted;
    };

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

void UI_Navigation::DrawSelectionLift(const ActiveLift& lift) const
{
    ImGui::TextColored(ImVec4(0.6f, 1.0f, 1.0f, 1.0f), "Lift");

    ImGui::Separator();

    ImGui::Text("%s", lift.TagName.c_str());

    ImGui::Text("%s", HexFormater::Hex32(lift.Handle).c_str());

    ImGui::Text("Position: %.2f, %.2f, %.2f", lift.Position[0],
        lift.Position[1], lift.Position[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", lift.Forward[0],
        lift.Forward[1], lift.Forward[2]);

    ImGui::Text("Up: %.2f, %.2f, %.2f", lift.Up[0], lift.Up[1], lift.Up[2]);

    ImGui::Text("Angle Type: %s", EnumToString::AngleTypeToString(
        lift.AngleType));

    ImGui::Text("Force Type: %s", EnumToString::ForceTypeToString(
        lift.ForceType));

    ImGui::Text("Launch Direction: %.2f, %.2f, %.2f",
        lift.LaunchDirection[0], lift.LaunchDirection[1],
        lift.LaunchDirection[2]);
}

void UI_Navigation::DrawSelectionShield(const ActiveShield& shield) const
{
    const char* typeStr = shield.Type == ShieldType::OneWay ?
        "One-Way" : shield.Type == ShieldType::TwoWay ?
        "Two-Way" : "Blocker";

    ImGui::TextColored(ImVec4(0.85f, 0.4f, 1.0f, 1.0f), "Shield");

    ImGui::Separator();

    ImGui::Text("%s", shield.TagName.c_str());

    ImGui::Text("%s", HexFormater::Hex32(shield.Handle).c_str());

    ImGui::Text("Position: %.2f, %.2f, %.2f", shield.Position[0],
        shield.Position[1], shield.Position[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", shield.Forward[0],
        shield.Forward[1], shield.Forward[2]);

    ImGui::Text("Up: %.2f, %.2f, %.2f", shield.Up[0],
        shield.Up[1], shield.Up[2]);

    ImGui::Text("Type: %s", typeStr);

    if (shield.BlockDirection.has_value())
    {
        ImGui::Text("Block Direction: %.2f, %.2f, %.2f",
            (*shield.BlockDirection)[0], (*shield.BlockDirection)[1],
            (*shield.BlockDirection)[2]);
    }
}

void UI_Navigation::DrawSelectionObjectiveSpawn(
    const ActiveObjectiveSpawn& spawn) const
{
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Objective Spawn");

    ImGui::Separator();

    ImGui::Text("%s", spawn.TagName.c_str());

    ImGui::Text("%s", HexFormater::Hex32(spawn.Handle).c_str());

    ImGui::Text("Position: %.2f, %.2f, %.2f", spawn.Position[0],
        spawn.Position[1], spawn.Position[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", spawn.Forward[0],
        spawn.Forward[1], spawn.Forward[2]);

    ImGui::Text("Up: %.2f, %.2f, %.2f", spawn.Up[0],
        spawn.Up[1], spawn.Up[2]);

    ImGui::Text("Team: %s", EnumToString::TeamToString(spawn.Team));

    ImGui::Spacing();

    this->DrawZoneShape(spawn.ZoneShape);
}

void UI_Navigation::DrawZoneShape(ZoneShape zoneShape) const
{
    ImGui::TextDisabled("Zone Shape:");
    switch (zoneShape.ShapeType)
    {
    case ShapeType::None:
    {
        ImGui::Text("None");
        break;
    }

    case ShapeType::Cylinder:
    {
        ImGui::Text("Cylinder");
        ImGui::Text("Radius: %.2f", zoneShape.Radius);
        ImGui::Text("Top: %.2f", zoneShape.Top);
        ImGui::Text("Bottom: %.2f", zoneShape.Bottom);
        break;
    }

    case ShapeType::Box:
        ImGui::Text("Box");
        ImGui::Text("Width: %.2f", zoneShape.Radius);
        ImGui::Text("Length: %.2f", zoneShape.Length);
        ImGui::Text("Top: %.2f", zoneShape.Top);
        ImGui::Text("Bottom: %.2f", zoneShape.Bottom);
        break;
    }
}

void UI_Navigation::DrawSelectionObjective(
    const ActiveObjective& objective) const
{
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s",
        objective.IsEquipped ? "Objective (Carried)" : "Objective");

    ImGui::Separator();

    ImGui::Text("%s", objective.TagName.c_str());

    ImGui::Text("%s", HexFormater::Hex32(objective.Handle).c_str());

    ImGui::Text("Position: %.2f, %.2f, %.2f", objective.Position[0],
        objective.Position[1], objective.Position[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", objective.Forward[0],
        objective.Forward[1], objective.Forward[2]);

    ImGui::Text("Up: %.2f, %.2f, %.2f", objective.Up[0],
        objective.Up[1], objective.Up[2]);

    ImGui::Text("Linear Velocity: %.2f, %.2f, %.2f",
        objective.LinearVelocity[0], objective.LinearVelocity[1],
        objective.LinearVelocity[2]);

    ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f",
        objective.AngularVelocity[0], objective.AngularVelocity[1],
        objective.AngularVelocity[2]);

    ImGui::Text("Team: %s", EnumToString::TeamToString(objective.Team));

    ImGui::Text("Is Equipped: %s", objective.IsEquipped ? "Yes" : "No");

    if (objective.IsEquipped)
    {
        ImGui::Text("Carrier Handle: %s", HexFormater::Hex32(
            objective.CarrierHandle));
    }
}

void UI_Navigation::DrawSelectionDestructible(
    const ActiveDestructible& destructible) const
{
    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Destructible");

    ImGui::Separator();

    ImGui::Text("%s", destructible.TagName.c_str());

    ImGui::Text("%s", HexFormater::Hex32(destructible.Handle).c_str());

    ImGui::Text("Position: %.2f, %.2f, %.2f", destructible.Position[0],
        destructible.Position[1], destructible.Position[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", destructible.Forward[0],
        destructible.Forward[1], destructible.Forward[2]);

    ImGui::Text("Up: %.2f, %.2f, %.2f", destructible.Up[0],
        destructible.Up[1], destructible.Up[2]);

    ImGui::Text("Linear Velocity: %.2f, %.2f, %.2f",
        destructible.LinearVelocity[0], destructible.LinearVelocity[1],
        destructible.LinearVelocity[2]);

    ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f",
        destructible.AngularVelocity[0], destructible.AngularVelocity[1],
        destructible.AngularVelocity[2]);

    ImGui::Text("Type: %s", EnumToString::DestructibleTypeToString(
        destructible.Type));

    ImGui::Text("Health: %.2f", destructible.Health);
}

void UI_Navigation::DrawSelectionTriggerVolume(
    const ScnrTriggerVolume& volume, bool isKill) const
{
    ImVec4 titleColor = isKill ?
        ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(0.3f, 1.0f, 0.3f, 1.0f);

    ImGui::TextColored(titleColor, isKill ? "Kill Zone" : "Safe Zone");

    ImGui::Separator();

    ImGui::Text("NameId: %s", HexFormater::Hex32(volume.NameId));

    ImGui::Text("Position: %.2f, %.2f, %.2f", volume.Position[0],
        volume.Position[1], volume.Position[2]);

    ImGui::Text("Extents: %.2f, %.2f, %.2f", volume.Extents[0],
        volume.Extents[1], volume.Extents[2]);

    ImGui::Text("Forward: %.2f, %.2f, %.2f", volume.Forward[0],
        volume.Forward[1], volume.Forward[2]);

    ImGui::Text("Up: %.2f, %.2f, %.2f", volume.Up[0],
        volume.Up[1], volume.Up[2]);

    ImGui::Spacing();

    ImGui::Text("Type: %s", volume.Type ==
        ScnrTriggerVolumeType::BoundingBox ? "Bounding Box" : "Sector");

    ImGui::Text("Only Kill Players: %s", volume.OnlyKillPlayers ?
        "Yes" : "No");
}