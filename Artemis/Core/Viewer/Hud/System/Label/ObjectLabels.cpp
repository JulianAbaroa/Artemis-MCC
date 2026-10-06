module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :ObjectLabels;

import Common.Math.Type;
import Egocentric.Affordance.Type;
import Gui.Format.System;
import Relations.Classifier.Type;
import Viewer.Options.State;
import Viewer.Options.Type;
import Viewer.Style.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Flag = Viewer::Options::Type::Flag;
    using Scalar = Viewer::Options::Type::Scalar;
    using Role = Relations::Classifier::Type::Role;
    using LabelRow = Viewer::Hud::Type::LabelRow;
    using HexFormater = Gui::Format::System::HexFormater;
    using RoleFormater = Gui::Format::System::RoleFormater;

    using Viewer::Style::Type::k_Lift;
    using Viewer::Style::Type::k_Health;
    using Viewer::Style::Type::k_Shield;
    using Viewer::Style::Type::k_Destructible;
    using Viewer::Style::Type::k_Teleporter;
    using Viewer::Style::Type::k_Collidable;
    using Viewer::Style::Type::k_Selected;

    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };
    constexpr float k_SelectedPriority{ 2.0f };
    constexpr float k_MinimumSpeed{ 1e-5f };

    constexpr ImU32 k_TagColor{ IM_COL32(255, 255, 255, 255) };
    constexpr ImU32 k_InfoColor{ IM_COL32(190, 200, 215, 255) };
    constexpr ImU32 k_MutedColor{ IM_COL32(150, 160, 175, 255) };
    constexpr ImU32 k_FreeSeatColor{ IM_COL32(120, 200, 130, 255) };
    constexpr ImU32 k_TakenSeatColor{ IM_COL32(215, 110, 110, 255) };

    enum class Category : std::uint8_t
    {
        Skip,
        Vehicle,
        Biped,
        Weapon,
        Equipment,
        Projectile,
        Other,
    };

    auto CategoryOf(Role role) -> Category
    {
        switch (role)
        {
        case Role::Unknown:
        case Role::None:
        case Role::VehiclePart:
        case Role::WeaponEquipped:
        case Role::ObjectiveEquipped:
        case Role::ArmorAbilityEquipped:
            return Category::Skip;

        case Role::Vehicle: return Category::Vehicle;
        case Role::Biped: return Category::Biped;

        case Role::WeaponPickup:
        case Role::ObjectivePickup:
            return Category::Weapon;

        case Role::ArmorAbilityPickup:
        case Role::GrenadePickup:
        case Role::AmmoPickup:
        case Role::Powerup:
            return Category::Equipment;

        case Role::Projectile: return Category::Projectile;

        default: return Category::Other;
        }
    }

    auto IsCategoryEnabled(Category category, const Viewer::Options::State::OptionsStore& options) -> bool
    {
        switch (category)
        {
        case Category::Vehicle: return options.IsEnabled(Flag::ObjectVehicles);
        case Category::Biped: return options.IsEnabled(Flag::ObjectBipeds);
        case Category::Weapon: return options.IsEnabled(Flag::ObjectWeapons);
        case Category::Equipment: return options.IsEnabled(Flag::ObjectEquipment);
        case Category::Projectile: return options.IsEnabled(Flag::ObjectProjectiles);
        case Category::Other: return options.IsEnabled(Flag::ObjectOther);
        default: return false;
        }
    }

    auto AccentOf(Category category) -> Viewer::Style::Type::Color
    {
        switch (category)
        {
        case Category::Vehicle: return k_Lift;
        case Category::Biped: return k_Health;
        case Category::Weapon: return k_Shield;
        case Category::Equipment: return k_Destructible;
        case Category::Projectile: return k_Teleporter;
        default: return k_Collidable;
        }
    }

    auto Magnitude(const Vec3& v) -> float
    {
        return std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
    }
}

namespace Viewer::Hud::System
{
    auto ObjectLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = context.Options;

        if (!tick.ObjectTable) return;

        const bool selectedOnly = options.IsEnabled(Flag::ObjectSelectedOnly);
        const bool showTag = options.IsEnabled(Flag::ObjectTagName);
        const bool showRole = options.IsEnabled(Flag::ObjectRole);
        const bool showHandle = options.IsEnabled(Flag::ObjectHandle);
        const bool showDistance = options.IsEnabled(Flag::ObjectDistance);
        const bool showLinear = options.IsEnabled(Flag::ObjectLinearVelocity);
        const bool showArrow = options.IsEnabled(Flag::ObjectVelocityArrow);
        const bool showAngular = options.IsEnabled(Flag::ObjectAngularVelocity);
        const bool showDamage = options.IsEnabled(Flag::ObjectDamage);
        const bool showParent = options.IsEnabled(Flag::ObjectParent);
        const bool showSeats = options.IsEnabled(Flag::VehicleSeatSummary);
        const bool showHijackers = options.IsEnabled(Flag::VehicleHijackerSlots);
        const float arrowLength = options.GetScalar(Scalar::ObjectArrowLength);

        std::unordered_map<std::uint32_t, const Egocentric::Affordance::Type::Affordance*> affordances{};
        if (showSeats && tick.Affordances)
        {
            for (const auto& affordance : *tick.Affordances)
            {
                if (!affordance.Seats.empty()) affordances[affordance.Handle] = &affordance;
            }
        }

        std::unordered_map<std::uint32_t, const std::string*> gamertagByBiped{};
        if (showSeats && tick.PlayerTable)
        {
            for (const auto& entry : *tick.PlayerTable)
            {
                gamertagByBiped[entry.second.AliveBipedHandle] = &entry.second.Gamertag;
            }
        }

        auto collect = [&](const auto& object, Category category, bool isSelected)
        {
            std::vector<LabelRow> rows{};

            if (showTag)
            {
                const std::string name = ShortTagName(object.TagName);
                rows.push_back(LabelRow{ name.empty() ? object.FourCC : name, k_TagColor });
            }

            if (showRole)
            {
                const auto roleIt = context.Roles.find(object.Handle);
                rows.push_back(LabelRow{ roleIt == context.Roles.end() ? "Unknown"
                    : RoleFormater::RoleToString(roleIt->second), k_InfoColor });
            }

            if (showHandle)
            {
                rows.push_back(LabelRow{ HexFormater::Hex32(object.Handle), k_MutedColor });
            }

            if (showParent && object.ParentHandle != k_NoHandle && object.ParentHandle != 0)
            {
                rows.push_back(LabelRow{ "parent " + HexFormater::Hex32(object.ParentHandle), k_MutedColor });
            }

            if (showDistance)
            {
                rows.push_back(DistanceRow(context, object.Position, k_MutedColor));
            }

            const float speed = Magnitude(object.LinearVelocity);

            if (showLinear)
            {
                rows.push_back(LabelRow{ std::format("v: {:.2f} wu/s", speed), k_InfoColor });
            }

            if (showAngular)
            {
                rows.push_back(LabelRow{ std::format("ang: {:.3f}", Magnitude(object.AngularVelocity)), k_InfoColor });
            }

            if (showDamage)
            {
                rows.push_back(LabelRow{ std::format("dmg: {:.2f}", object.DamageReceived), k_InfoColor });
            }

            if (showSeats && category == Category::Vehicle)
            {
                const auto affordanceIt = affordances.find(object.Handle);
                if (affordanceIt != affordances.end())
                {
                    for (const auto& seat : affordanceIt->second->Seats)
                    {
                        if (seat.IsHijackerSlot && !showHijackers) continue;

                        std::string state = "free";
                        if (seat.IsOccupied)
                        {
                            state = "occupied";

                            const auto nameIt = gamertagByBiped.find(seat.OccupyingBipedHandle);
                            if (nameIt != gamertagByBiped.end() && !nameIt->second->empty())
                            {
                                state = *nameIt->second;
                            }
                        }

                        rows.push_back(LabelRow{ seat.SeatName + ": " + state,
                            seat.IsOccupied ? k_TakenSeatColor : k_FreeSeatColor });
                    }
                }
            }

            const Vec3 anchor{ object.Position.X, object.Position.Y,
                object.Position.Z + object.CurrentRadius + 0.1f };

            const ImU32 accent = isSelected
                ? ToImColor(k_Selected) : ToImColor(AccentOf(category));

            canvas.Add(anchor, std::move(rows), accent, isSelected ? k_SelectedPriority : 0.0f, object.Handle);

            if (showArrow && speed > k_MinimumSpeed)
            {
                const float scale = arrowLength / speed;

                const Vec3 tip{
                    object.Position.X + object.LinearVelocity.X * scale,
                    object.Position.Y + object.LinearVelocity.Y * scale,
                    object.Position.Z + object.LinearVelocity.Z * scale };

                canvas.AddArrow(object.Position, tip, accent);
            }
        };

        if (context.Selected != k_NoHandle)
        {
            const auto selectedIt = tick.ObjectTable->find(context.Selected);
            if (selectedIt != tick.ObjectTable->end())
            {
                const auto roleIt = context.Roles.find(context.Selected);
                const Category category = roleIt == context.Roles.end()
                    ? Category::Other : CategoryOf(roleIt->second);

                collect(selectedIt->second, category, true);
            }
        }

        if (selectedOnly) return;

        for (const auto& entry : *tick.ObjectTable)
        {
            const auto& object = entry.second;
            if (object.Handle == context.Selected) continue;

            const auto roleIt = context.Roles.find(object.Handle);
            if (roleIt == context.Roles.end()) continue;

            const Category category = CategoryOf(roleIt->second);
            if (!IsCategoryEnabled(category, options)) continue;

            collect(object, category, false);
        }
    }
}