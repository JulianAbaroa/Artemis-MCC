module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Player;

import Blam.Layout.Player.Type;
import Perception.Match.Type;
import DevTools.Gui.Format.System;
import DevTools.Viewer.Format.System;
import std;

namespace
{
    using Blam::Layout::Player::Type::Traits::PlayerTraits;
    using Perception::Match::Type::MatchScore;
    using Perception::Players::Type::Player;

    using DevTools::Gui::Format::System::HexFormater;
    using DevTools::Viewer::Format::System::PlayerFormater;

    auto Row(const char* label, const std::string& value) -> void
    {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);

        ImGui::TableNextColumn();
        ImGui::TextUnformatted(value.c_str());
    }

    auto Multiplier(float value) -> std::string
    {
        return std::format("x{:.2f}", value);
    }

    auto YesNo(bool value) -> const char*
    {
        return value ? "Yes" : "No";
    }

    // return: The raw index of a table of the game variant editor, or none for -1.
    auto TableIndex(std::int8_t value) -> std::string
    {
        return value < 0 ? std::string{ "none" } : std::to_string(value);
    }

    auto DrawTraits(const PlayerTraits& traits) -> void
    {
        if (!ImGui::BeginTable("##traits", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) return;

        ImGui::TableSetupColumn("Trait");
        ImGui::TableSetupColumn("Value");
        ImGui::TableHeadersRow();

        Row("Damage resistance", Multiplier(traits.DamageResistance));
        Row("Health multiplier", Multiplier(traits.HealthMultiplier));
        Row("Health regen rate", Multiplier(traits.HealthRegenRate));
        Row("Shield multiplier", Multiplier(traits.ShieldMultiplier));
        Row("Shield regen rate", Multiplier(traits.ShieldRegenRate));
        Row("Overshield regen rate", Multiplier(traits.OvershieldRegenRate));
        Row("Shield vampirism", Multiplier(traits.ShieldVampirism));
        Row("Damage multiplier", Multiplier(traits.DamageMultiplier));
        Row("Melee multiplier", Multiplier(traits.MeleeMultiplier));
        Row("Movement speed", Multiplier(traits.MovementSpeed));
        Row("Gravity", Multiplier(traits.Gravity));
        Row("Jump height", Multiplier(traits.JumpHeight));
        Row("Radar range", std::format("{:.0f} m", traits.RadarRange));
        Row("Headshot immune", YesNo(traits.IsHeadshotImmune));
        Row("Assassination immune", YesNo(traits.IsAssassinationImmune));
        Row("Cannot die from damage", YesNo(traits.CannotDieFromDamage));
        Row("Grenade regen", YesNo(traits.IsGrenadeRegenOn));
        Row("Weapon pickup", YesNo(traits.IsWeaponPickupOn));
        Row("Abilities drop on death", YesNo(traits.DoAbilitiesDropOnDeath));
        Row("Infinite abilities", YesNo(traits.AreAbilitiesInfinite));
        Row("Infinite ammo", PlayerFormater::InfiniteAmmoToString(traits.InfiniteAmmo));
        Row("Ability usage", PlayerFormater::AbilityUsageToString(traits.AbilityUsage));
        Row("Vehicle use", PlayerFormater::VehicleUseToString(traits.VehicleUse));
        Row("Visible waypoint", PlayerFormater::IdentityVisibilityToString(traits.VisibleWaypoint));
        Row("Visible name", PlayerFormater::IdentityVisibilityToString(traits.VisibleName));
        Row("Radar mode", PlayerFormater::RadarModeToString(traits.RadarMode));
        Row("Frag grenades", std::to_string(traits.FragGrenades));
        Row("Plasma grenades", std::to_string(traits.PlasmaGrenades));
        Row("Active camo (raw)", std::to_string(traits.ActiveCamo));
        Row("Primary weapon (index)", TableIndex(traits.PrimaryWeapon));
        Row("Secondary weapon (index)", TableIndex(traits.SecondaryWeapon));
        Row("Ability (index)", TableIndex(traits.Ability));

        ImGui::EndTable();
    }
}

namespace DevTools::Viewer::Inspector::System
{
    auto PlayerSection::Draw(const Tick& tick, const Player& player) -> void
    {
        ImGui::Text("Handle: %s", HexFormater::Hex32(player.Handle).c_str());
        ImGui::Text("Connection: %s", PlayerFormater::ConnectionToString(player.Connection));

        if (player.LocalIndex) ImGui::Text("Local index: %d", static_cast<int>(*player.LocalIndex));
        else                   ImGui::TextDisabled("Local index: none");

        ImGui::Text("Respawn ticks: %u", static_cast<unsigned>(player.RespawnTicks));
        ImGui::Text("Powerup: %s (%u ticks)", PlayerFormater::PowerupToString(player.Powerup), static_cast<unsigned>(player.PowerupTicks));

        // note: The score is in the match status, by player handle.
        const MatchScore* score{ nullptr };

        if (tick.Match)
        {
            for (const MatchScore& entry : tick.Match->Scores)
            {
                if (entry.PlayerHandle == player.Handle) score = &entry;
            }
        }

        if (score) ImGui::Text("Score: %d", static_cast<int>(score->Score));
        else       ImGui::TextDisabled("Score: none");

        ImGui::SeparatorText("Traits");
        DrawTraits(player.Traits);
    }
}