module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :ObjectLabels;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Perception.Classifier.Type;
import Views.Affordance.Type;
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
    using DevTools::Viewer::Options::Type::Scalar;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::Binding;
    using Perception::Classifier::Type::Classified;
    using DevTools::Viewer::Hud::Type::LabelRow;
    using DevTools::Gui::Format::System::HexFormater;

    namespace SemanticType = Common::Semantic::Type;

    using DevTools::Viewer::Style::Type::k_Health;
    using DevTools::Viewer::Style::Type::k_Affordance;
    using DevTools::Viewer::Style::Type::k_Collidable;
    using DevTools::Viewer::Style::Type::k_Selected;
    using Common::Engine::Type::k_NoHandle;
    using DevTools::Viewer::Hud::Type::k_InfoColor;
    using DevTools::Viewer::Hud::Type::k_MutedColor;
    using DevTools::Viewer::Hud::Type::k_FreeSeatColor;
    using DevTools::Viewer::Hud::Type::k_TakenSeatColor;

    // Above the other selected labels: the object label is the main one.
    constexpr float k_SelectedObjectPriority{ 2.0f };
    constexpr float k_MinimumSpeed{ 1e-5f };

    constexpr ImU32 k_TagColor{ IM_COL32(255, 255, 255, 255) };

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

    // Returns the label category of an object. Held, seated and attached objects are skipped: their holder carries the label.
    auto CategoryOf(const Classified& classified) -> Category
    {
        if (classified.Binding != Binding::Free) return Category::Skip;

        switch (classified.Function)
        {
        case Function::Unknown:
        case Function::Decor:
        case Function::CameraMarker:
            return Category::Skip;

        case Function::Vehicle:
        case Function::Turret:
            return Category::Vehicle;

        case Function::Combatant:
        case Function::Decoy:
            return Category::Biped;

        case Function::Weapon:
        case Function::ObjectiveItem:
            return Category::Weapon;

        case Function::ArmorAbility:
        case Function::Grenade:
        case Function::AmmoPack:
        case Function::Powerup:
            return Category::Equipment;

        case Function::Projectile: return Category::Projectile;

        default: return Category::Other;
        }
    }

    // Returns the text of the function row: function, subtype if any, and binding if the object is not free.
    auto FunctionText(const Classified& classified) -> std::string
    {
        std::string text{ SemanticType::NameOf(classified.Function) };

        const std::string_view subtype{ SemanticType::Subtype::NameOf(classified.Function, classified.Subtype) };
        if (!subtype.empty()) text += std::format(" {}", subtype);

        if (classified.Binding != Binding::Free) text += std::format(" ({})", SemanticType::NameOf(classified.Binding));

        return text;
    }

    auto IsCategoryEnabled(Category category, const DevTools::Viewer::Options::State::OptionsStore& options) -> bool
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

    auto GroupOf(Category category) -> DevTools::Viewer::Style::Type::Color
    {
        switch (category)
        {
        case Category::Vehicle:
        case Category::Weapon:
        case Category::Equipment: return k_Affordance;
        case Category::Biped: return k_Health;
        default: return k_Collidable;
        }
    }

    auto Magnitude(const Vec3& v) -> float
    {
        return std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
    }
}

namespace DevTools::Viewer::Hud::System
{
    auto ObjectLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = canvas.GetOptions();

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

        std::unordered_map<std::uint32_t, const Views::Affordance::Type::Affordance*> affordances{};
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
                const auto classifiedIt = context.Classifications.find(object.Handle);
                rows.push_back(LabelRow{ classifiedIt == context.Classifications.end() ? std::string{ "Unknown" }
                    : FunctionText(*classifiedIt->second), k_InfoColor });
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
                rows.push_back(DistanceRow(context, object.BoundingCenter, k_MutedColor));
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

            const Vec3 anchor{ object.BoundingCenter.X, object.BoundingCenter.Y,
                object.BoundingCenter.Z + object.BoundingRadius + 0.1f };

            const ImU32 group = isSelected
                ? ToImColor(k_Selected) : ToImColor(GroupOf(category));

            canvas.Add(anchor, std::move(rows), group, TeamColorOf(context, object.Handle),
                isSelected ? k_SelectedObjectPriority : 0.0f, object.Handle);

            if (showArrow && speed > k_MinimumSpeed)
            {
                const float scale = arrowLength / speed;

                const Vec3 tip{
                    object.BoundingCenter.X + object.LinearVelocity.X * scale,
                    object.BoundingCenter.Y + object.LinearVelocity.Y * scale,
                    object.BoundingCenter.Z + object.LinearVelocity.Z * scale };

                canvas.AddArrow(object.BoundingCenter, tip, group);
            }
        };

        if (context.Selected != k_NoHandle)
        {
            const auto selectedIt = tick.ObjectTable->find(context.Selected);
            if (selectedIt != tick.ObjectTable->end())
            {
                const auto classifiedIt = context.Classifications.find(context.Selected);
                const Category category = classifiedIt == context.Classifications.end()
                    ? Category::Other : CategoryOf(*classifiedIt->second);

                collect(selectedIt->second, category, true);
            }
        }

        if (selectedOnly) return;

        for (const auto& entry : *tick.ObjectTable)
        {
            const auto& object = entry.second;
            if (object.Handle == context.Selected) continue;

            const auto classifiedIt = context.Classifications.find(object.Handle);
            if (classifiedIt == context.Classifications.end()) continue;

            const Category category = CategoryOf(*classifiedIt->second);
            if (!IsCategoryEnabled(category, options)) continue;

            collect(object, category, false);
        }
    }
}