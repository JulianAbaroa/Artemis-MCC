module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :PlayerLabels;

import Common.Math.Type;
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
    using PlayerTree = Export::Tick::Type::PlayerTree;
    using LabelRow = Viewer::Hud::Type::LabelRow;

    using Viewer::Style::Type::k_Selected;

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

        if (!tick.PlayerTable || !tick.ObjectTable) return;

        const bool showGamertag = options.IsEnabled(Flag::PlayerGamertag);
        const bool useTeamColor = options.IsEnabled(Flag::PlayerTeamColor);
        const bool showDistance = options.IsEnabled(Flag::PlayerDistance);
        const bool showPrimary = options.IsEnabled(Flag::PlayerWeaponPrimary);
        const bool showSecondary = options.IsEnabled(Flag::PlayerWeaponSecondary);
        const bool showHeld = options.IsEnabled(Flag::PlayerWeaponHeld);
        const bool showEquipped = options.IsEnabled(Flag::PlayerWeaponEquipped);
        const bool showSelf = options.IsEnabled(Flag::PlayerSelf);

        if (!showGamertag && !showDistance && !showPrimary && !showSecondary && !showHeld && !showEquipped) return;

        auto treeOf = [&](std::uint32_t playerHandle) -> const PlayerTree*
        {
            if (!tick.PlayerGraph) return nullptr;

            for (const auto& tree : *tick.PlayerGraph)
            {
                if (tree.Handle == playerHandle) return &tree;
            }

            return nullptr;
        };

        auto heldNames = [&](const PlayerTree* tree) -> std::vector<std::string>
        {
            std::vector<std::string> names{};
            if (!tree) return names;

            for (const std::uint32_t weapon : tree->HeldWeaponHandles)
            {
                const auto objectIt = tick.ObjectTable->find(weapon);
                if (objectIt == tick.ObjectTable->end()) continue;

                names.push_back(ShortTagName(objectIt->second.TagName));
            }

            return names;
        };

        auto weaponName = [&](std::uint32_t handle) -> std::string
        {
            if (handle == k_NoHandle) return "none";

            const auto it = tick.ObjectTable->find(handle);
            if (it == tick.ObjectTable->end()) return "missing";

            return ShortTagName(it->second.TagName);
        };

        for (const auto& entry : *tick.PlayerTable)
        {
            const auto& player = entry.second;

            const auto bipedIt = tick.ObjectTable->find(player.AliveBipedHandle);
            if (bipedIt == tick.ObjectTable->end()) continue;

            const bool isSelf = player.AliveBipedHandle == context.SelfBiped;
            if (!showSelf && isSelf) continue;

            const auto& biped = bipedIt->second;

            Vec3 anchor{ biped.Position.X, biped.Position.Y, biped.Position.Z + 0.8f };
            bool hasAnchor{ false };

            if (tick.Aims)
            {
                const auto aimIt = tick.Aims->find(player.AliveBipedHandle);
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

            const Viewer::Style::Type::Color teamColor = Viewer::Style::Type::ColorOfTeam(player.Team);
            const ImU32 accent = ToImColor(useTeamColor ? teamColor : k_Selected);

            std::vector<LabelRow> rows{};

            if (showGamertag)
            {
                const std::string& name = player.Gamertag.empty() ? player.Tag : player.Gamertag;

                rows.push_back(LabelRow{ name.empty() ? std::string{ "Player" } : name,
                    useTeamColor ? ToImColor(teamColor) : IM_COL32(255, 255, 255, 255) });
            }

            std::vector<std::string> held{};
            if (showHeld || showEquipped) held = heldNames(treeOf(entry.first));

            if (showEquipped)
            {
                rows.push_back(LabelRow{ "W: " + (held.empty() ? std::string{ "none" } : held.front()), k_WeaponColor });
            }

            if (showPrimary)
            {
                rows.push_back(LabelRow{ "P: " + weaponName(player.PrimaryWeaponHandle), k_WeaponColor });
            }

            if (showSecondary)
            {
                rows.push_back(LabelRow{ "S: " + weaponName(player.SecondaryWeaponHandle), k_WeaponColor });
            }

            if (showHeld)
            {
                std::string joined{};
                for (const std::string& name : held)
                {
                    if (!joined.empty()) joined += ", ";
                    joined += name;
                }

                rows.push_back(LabelRow{ "H: " + (joined.empty() ? std::string{ "none" } : joined), k_WeaponColor });
            }

            if (showDistance && !isSelf)
            {
                rows.push_back(DistanceRow(context, biped.Position, k_MutedColor));
            }

            canvas.Add(anchor, std::move(rows), accent,
                player.AliveBipedHandle == context.Selected ? k_SelectedPriority : 0.0f,
                player.AliveBipedHandle);
        }
    }
}