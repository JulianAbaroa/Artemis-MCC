module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :InteractionLabels;

import Common.Math.Type;
import Egocentric.Affordance.Type;
import Gui.Format.System;
import Viewer.Options.State;
import Viewer.Options.Type;
import Viewer.Style.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Flag = Viewer::Options::Type::Flag;
    using LabelRow = Viewer::Hud::Type::LabelRow;
    using InteractionFormater = Gui::Format::System::InteractionFormater;
    using AffordanceFormater = Gui::Format::System::AffordanceFormater;
    using RoleFormater = Gui::Format::System::RoleFormater;
    using Affordance = Egocentric::Affordance::Type::Affordance;

    using Viewer::Style::Type::k_InteractionObject;
    using Viewer::Style::Type::k_InteractionMelee;
    using Viewer::Style::Type::k_InteractionAim;
    using Viewer::Style::Type::k_Affordance;

    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };
    constexpr float k_TargetPriority{ 3.0f };
    constexpr float k_LinkLift{ 0.5f };

    constexpr ImU32 k_InfoColor{ IM_COL32(190, 200, 215, 255) };
    constexpr ImU32 k_MutedColor{ IM_COL32(150, 160, 175, 255) };

    auto JoinBehaviors(const Affordance& affordance) -> std::string
    {
        std::string joined{};

        for (const auto behavior : affordance.Behaviors)
        {
            if (!joined.empty()) joined += ", ";
            joined += AffordanceFormater::BehaviorToString(behavior);
        }

        return joined.empty() ? std::string{ "none" } : joined;
    }
}

namespace Viewer::Hud::System
{
    auto InteractionLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = context.Options;

        const bool showObject = options.IsEnabled(Flag::InteractionObject);
        const bool showMelee = options.IsEnabled(Flag::InteractionMelee);
        const bool showAim = options.IsEnabled(Flag::InteractionAim);
        const bool showHit = options.IsEnabled(Flag::InteractionAimHitPoint);
        const bool showLink = options.IsEnabled(Flag::InteractionLink);
        const bool showAffordance = options.IsEnabled(Flag::InteractionAffordanceDetails);
        const bool showAllAffordances = options.IsEnabled(Flag::AffordanceAll);

        if (!tick.ObjectTable) return;

        auto distanceRow = [&](const Vec3& position) -> LabelRow
        {
            return DistanceRow(context, position, k_MutedColor);
        };

        auto findAffordance = [&](std::uint32_t handle) -> const Affordance*
        {
            if (!tick.Affordances) return nullptr;

            for (const auto& affordance : *tick.Affordances)
            {
                if (affordance.Handle == handle) return &affordance;
            }

            return nullptr;
        };

        auto mark = [&](std::uint32_t handle, std::vector<LabelRow> rows,
            const Viewer::Style::Type::Color& color) -> void
        {
            const auto objectIt = tick.ObjectTable->find(handle);
            if (objectIt == tick.ObjectTable->end()) return;

            const auto& object = objectIt->second;

            const Vec3 anchor{ object.Position.X, object.Position.Y,
                object.Position.Z + object.BoundingRadius + 0.1f };

            canvas.Add(anchor, std::move(rows), ToImColor(color), k_TargetPriority, handle);

            if (showLink && context.HasSelf)
            {
                const Vec3 origin{ context.SelfPosition.X, context.SelfPosition.Y,
                    context.SelfPosition.Z + k_LinkLift };

                canvas.AddArrow(origin, object.Position, ToImColor(color));
            }
        };

        const std::uint32_t engineTarget = (tick.Interaction && showObject
            && tick.Interaction->HasObjectTarget())
            ? tick.Interaction->TargetObjectHandle : k_NoHandle;

        if (tick.Interaction)
        {
            const auto& interaction = *tick.Interaction;

            if (engineTarget != k_NoHandle)
            {
                const auto objectIt = tick.ObjectTable->find(engineTarget);

                std::vector<LabelRow> rows{};
                rows.push_back(LabelRow{ "ENGINE TARGET", ToImColor(k_InteractionObject) });
                rows.push_back(LabelRow{ InteractionFormater::InteractionKindToString(interaction.Kind), k_InfoColor });
                rows.push_back(LabelRow{ InteractionFormater::InteractionDetailToString(
                    interaction.Kind, interaction.InteractionSlotID), k_InfoColor });

                if (objectIt != tick.ObjectTable->end())
                {
                    rows.push_back(distanceRow(objectIt->second.Position));
                }

                if (showAffordance)
                {
                    if (const Affordance* affordance = findAffordance(engineTarget))
                    {
                        rows.push_back(LabelRow{ "Behaviors: " + JoinBehaviors(*affordance), k_InfoColor });
                        rows.push_back(LabelRow{ std::string{ "Activation: " } +
                            AffordanceFormater::ActivationToString(affordance->Activation), k_InfoColor });
                    }
                }

                mark(engineTarget, std::move(rows), k_InteractionObject);
            }

            if (showMelee && interaction.HasMeleeTarget())
            {
                std::vector<LabelRow> rows{};
                rows.push_back(LabelRow{ "MELEE TARGET", ToImColor(k_InteractionMelee) });

                const auto objectIt = tick.ObjectTable->find(interaction.MeleeTargetHandle);
                if (objectIt != tick.ObjectTable->end())
                {
                    rows.push_back(distanceRow(objectIt->second.Position));
                }

                mark(interaction.MeleeTargetHandle, std::move(rows), k_InteractionMelee);
            }

            if (showAim && interaction.HasAimTarget())
            {
                std::vector<LabelRow> rows{};
                rows.push_back(LabelRow{ "AIM TARGET", ToImColor(k_InteractionAim) });
                rows.push_back(LabelRow{ std::format("slot: 0x{:X}", interaction.AimTargetSlotID), k_InfoColor });
                rows.push_back(LabelRow{ std::format("part: 0x{:02X}", interaction.ModelPart), k_InfoColor });

                const auto objectIt = tick.ObjectTable->find(interaction.AimTargetHandle);
                if (objectIt != tick.ObjectTable->end())
                {
                    rows.push_back(distanceRow(objectIt->second.Position));
                }

                mark(interaction.AimTargetHandle, std::move(rows), k_InteractionAim);
            }
        }

        if (showHit && tick.Raycasts && tick.Raycasts->AimHit.Hit)
        {
            const auto& hit = tick.Raycasts->AimHit;

            std::vector<LabelRow> rows{};
            rows.push_back(LabelRow{ "AIM HIT", ToImColor(k_InteractionAim) });
            rows.push_back(LabelRow{ std::format("d: {:.1f} wu", hit.Distance), k_MutedColor });

            canvas.Add(hit.Point, std::move(rows), ToImColor(k_InteractionAim), k_TargetPriority);
        }

        if (showAllAffordances && tick.Affordances)
        {
            for (const auto& affordance : *tick.Affordances)
            {
                if (affordance.Handle == engineTarget) continue;

                std::vector<LabelRow> rows{};
                rows.push_back(LabelRow{ RoleFormater::RoleToString(affordance.Role),
                    ToImColor(k_Affordance) });
                rows.push_back(LabelRow{ "Behaviors: " + JoinBehaviors(affordance), k_InfoColor });
                rows.push_back(LabelRow{ std::string{ "Activation: " } +
                    AffordanceFormater::ActivationToString(affordance.Activation), k_InfoColor });
                rows.push_back(LabelRow{ std::format("d: {:.1f} wu", affordance.DistanceToPlayer), k_MutedColor });

                canvas.Add(affordance.Position, std::move(rows),
                    ToImColor(k_Affordance), 0.0f, affordance.Handle);
            }
        }
    }
}