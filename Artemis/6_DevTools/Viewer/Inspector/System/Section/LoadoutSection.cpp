module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Inspector.System;
import :Loadout;

import Common.Engine.Type;
import DevTools.Viewer.Format.System;
import std;

namespace
{
    using Bridge::Codec::Tick::Type::Tick;
    using DevTools::Viewer::Format::System::PlayerFormater;
    using DevTools::Viewer::Inspector::System::ObjectName;

    using Common::Engine::Type::k_NoHandle;

    auto SlotText(const Tick& tick, std::uint32_t weapon, std::uint32_t equipped) -> std::string
    {
        const bool isEquipped{ weapon != k_NoHandle && weapon == equipped };

        return std::format("{}{}", ObjectName::Label(tick, weapon), isEquipped ? " [equipped]" : "");
    }
}

namespace DevTools::Viewer::Inspector::System
{
    auto LoadoutSection::Draw(const Tick& tick, const Loadout& loadout) -> void
    {
        ImGui::TextWrapped("Primary: %s", SlotText(tick, loadout.PrimaryWeapon, loadout.EquippedWeapon).c_str());
        ImGui::TextWrapped("Secondary: %s", SlotText(tick, loadout.SecondaryWeapon, loadout.EquippedWeapon).c_str());
        ImGui::TextWrapped("Equipped: %s", ObjectName::Label(tick, loadout.EquippedWeapon).c_str());

        ImGui::Spacing();

        ImGui::TextWrapped("Ability: %s", ObjectName::Label(tick, loadout.Ability).c_str());

        if (loadout.Ability != k_NoHandle)
        {
            ImGui::Text("Ability energy: %.2f%s", loadout.AbilityEnergy, loadout.IsAbilityActive ? " (active)" : "");
        }

        ImGui::Spacing();

        ImGui::Text("Grenades: frag %u | plasma %u", static_cast<unsigned>(loadout.FragGrenades), static_cast<unsigned>(loadout.PlasmaGrenades));
        ImGui::Text("Selected grenade: %s", PlayerFormater::GrenadeToString(loadout.SelectedGrenade));

        ImGui::Spacing();

        ImGui::TextWrapped("Objective: %s", ObjectName::Label(tick, loadout.Objective).c_str());
    }
}