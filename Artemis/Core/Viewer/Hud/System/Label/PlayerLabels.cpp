module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :PlayerLabels;

import Common.Math.Type;
import Common.Semantic.Type;
import Export.Tick.Type;
import Resolved.World.Type;
import Viewer.Options.Type;
import Viewer.Style.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Flag = Viewer::Options::Type::Flag;
    using AnchorSource = Resolved::World::Type::ModelLink::AnchorSource;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Binding;
    using Export::Tick::Type::Entity;
    using Export::Tick::Type::Player;
    using Export::Tick::Type::Body;
    using LabelRow = Viewer::Hud::Type::LabelRow;

    using Viewer::Style::Type::k_Health;
    using Viewer::Style::Type::ColorOfTeam;

    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };
    constexpr float k_HeadClearance{ 0.12f };
    constexpr float k_FallbackEyeClearance{ 0.25f };
    constexpr float k_SelectedPriority{ 1.0f };

    constexpr ImU32 k_WeaponColor{ IM_COL32(190, 200, 215, 255) };
    constexpr ImU32 k_MutedColor{ IM_COL32(150, 160, 175, 255) };
}

namespace Viewer::Hud::System
{
    auto PlayerLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = context.Options;

        if (!tick.Players || !tick.Entities || !tick.ObjectTable) return;

        const bool showGamertag = options.IsEnabled(Flag::PlayerGamertag);
        const bool useTeamColor = options.IsEnabled(Flag::PlayerTeamColor);
        const bool showDistance = options.IsEnabled(Flag::PlayerDistance);
        const bool showVitality = options.IsEnabled(Flag::PlayerVitality);
        const bool showPrimary = options.IsEnabled(Flag::PlayerWeaponPrimary);
        const bool showSecondary = options.IsEnabled(Flag::PlayerWeaponSecondary);
        const bool showHeld = options.IsEnabled(Flag::PlayerWeaponHeld);
        const bool showEquipped = options.IsEnabled(Flag::PlayerWeaponEquipped);
        const bool showSelf = options.IsEnabled(Flag::PlayerSelf);

        if (!showGamertag && !showDistance && !showVitality && !showPrimary && !showSecondary && !showHeld && !showEquipped) return;

        // note: Weapons carried by each biped, to compare against the slots of the loadout.
        std::unordered_map<std::uint32_t, std::vector<std::uint32_t>> heldByBiped{};
        if (showHeld)
        {
            for (const Entity& entity : tick.Entities->Rows)
            {
                if (entity.Function != Function::Weapon || entity.Binding != Binding::Carried) continue;

                heldByBiped[entity.BoundTo].push_back(entity.Handle);
            }
        }

        auto weaponName = [&](std::uint32_t handle) -> std::string
        {
            if (handle == k_NoHandle) return "none";

            const auto it = tick.ObjectTable->find(handle);
            if (it == tick.ObjectTable->end()) return "missing";

            return ShortTagName(it->second.TagName);
        };

        for (const Player& player : tick.Players->Rows)
        {
            if (!player.IsAlive()) continue;

            const Entity* biped = tick.Entities->Find(player.BipedHandle);
            if (!biped) continue;

            const bool isSelf = player.BipedHandle == context.SelfBiped;
            if (!showSelf && isSelf) continue;

            const Body& body = *player.Body;

            Vec3 anchor{ biped->BoundingCenter.X, biped->BoundingCenter.Y, biped->BoundingCenter.Z + 0.8f };
            bool hasAnchor{ false };

            if (tick.Aims)
            {
                const auto aimIt = tick.Aims->find(player.BipedHandle);
                if (aimIt != tick.Aims->end())
                {
                    for (const auto& section : aimIt->second.Sections)
                    {
                        if (!section.Valid || section.AimSource != AnchorSource::HeadshotTarget) continue;

                        anchor = Vec3{ section.Position.X, section.Position.Y,
                            section.Position.Z + section.Radius + k_HeadClearance };
                        hasAnchor = true;
                        break;
                    }
                }
            }

            if (!hasAnchor)
            {
                const Vec3& eye = player.CameraPosition;
                if (eye.X != 0.0f || eye.Y != 0.0f || eye.Z != 0.0f)
                {
                    anchor = Vec3{ eye.X, eye.Y, eye.Z + k_FallbackEyeClearance };
                }
            }

            const ImU32 teamColor = ToImColor(ColorOfTeam(player.Team));

            std::vector<LabelRow> rows{};

            if (showGamertag)
            {
                rows.push_back(LabelRow{ player.Name.empty() ? std::string{ "Player" } : player.Name,
                    useTeamColor ? teamColor : IM_COL32(255, 255, 255, 255) });
            }

            if (showVitality)
            {
                const std::string shield = body.Shield ? std::format("{:.2f}", *body.Shield) : std::string{ "-" };
                const std::string health = biped->Vitality ? std::format("{:.2f}", *biped->Vitality) : std::string{ "-" };

                rows.push_back(LabelRow{ "Sh: " + shield + "  Hp: " + health, ToImColor(k_Health) });
            }

            if (showEquipped)
            {
                rows.push_back(LabelRow{ "W: " + weaponName(body.Loadout.EquippedWeapon), k_WeaponColor });
            }

            if (showPrimary)
            {
                rows.push_back(LabelRow{ "P: " + weaponName(body.Loadout.PrimaryWeapon), k_WeaponColor });
            }

            if (showSecondary)
            {
                rows.push_back(LabelRow{ "S: " + weaponName(body.Loadout.SecondaryWeapon), k_WeaponColor });
            }

            if (showHeld)
            {
                std::string joined{};

                const auto heldIt = heldByBiped.find(player.BipedHandle);
                if (heldIt != heldByBiped.end())
                {
                    for (const std::uint32_t weapon : heldIt->second)
                    {
                        if (!joined.empty()) joined += ", ";
                        joined += weaponName(weapon);
                    }
                }

                rows.push_back(LabelRow{ "H: " + (joined.empty() ? std::string{ "none" } : joined), k_WeaponColor });
            }

            if (showDistance && !isSelf)
            {
                rows.push_back(DistanceRow(context, biped->BoundingCenter, k_MutedColor));
            }

            canvas.Add(anchor, std::move(rows), ToImColor(k_Health), teamColor,
                player.BipedHandle == context.Selected ? k_SelectedPriority : 0.0f,
                player.BipedHandle);
        }
    }
}