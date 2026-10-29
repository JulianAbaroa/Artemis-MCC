module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :InteractionLabels;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Views.Affordance.Type;
import DevTools.Viewer.Format.System;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Style.Type;
import DevTools.Viewer.Hud.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using DevTools::Viewer::Options::Type::Flag;
    using DevTools::Viewer::Hud::Type::LabelRow;
    using DevTools::Viewer::Format::System::InteractionFormater;
    using DevTools::Viewer::Format::System::AffordanceFormater;
    using Common::Semantic::Type::NameOf;
    using Views::Affordance::Type::Affordance;

    using DevTools::Viewer::Style::Type::k_InteractionObject;
    using DevTools::Viewer::Style::Type::k_InteractionMelee;
    using DevTools::Viewer::Style::Type::k_InteractionAim;
    using DevTools::Viewer::Style::Type::k_Affordance;
    using DevTools::Viewer::Style::Type::k_TeamNeutral;
    using Common::Engine::Type::k_NoHandle;
    using DevTools::Viewer::Hud::Type::k_InfoColor;
    using DevTools::Viewer::Hud::Type::k_MutedColor;

    constexpr float k_TargetPriority{ 3.0f };
    constexpr float k_LinkLift{ 0.5f };

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

namespace DevTools::Viewer::Hud::System
{
    auto InteractionLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = canvas.GetOptions();

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
            const DevTools::Viewer::Style::Type::Color& color) -> void
        {
            const auto objectIt = tick.ObjectTable->find(handle);
            if (objectIt == tick.ObjectTable->end()) return;

            const auto& object = objectIt->second;

            const Vec3 anchor{ object.BoundingCenter.X, object.BoundingCenter.Y,
                object.BoundingCenter.Z + object.BoundingRadius + 0.1f };

            canvas.Add(anchor, std::move(rows), ToImColor(color),
                TeamColorOf(context, handle), k_TargetPriority, handle);

            if (showLink && context.HasSelf)
            {
                const Vec3 origin{ context.SelfPosition.X, context.SelfPosition.Y,
                    context.SelfPosition.Z + k_LinkLift };

                canvas.AddArrow(origin, object.BoundingCenter, ToImColor(color));
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
                    rows.push_back(distanceRow(objectIt->second.BoundingCenter));
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
                    rows.push_back(distanceRow(objectIt->second.BoundingCenter));
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
                    rows.push_back(distanceRow(objectIt->second.BoundingCenter));
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

            canvas.Add(hit.Point, std::move(rows), ToImColor(k_InteractionAim),
                ToImColor(k_TeamNeutral), k_TargetPriority);
        }

        if (showAllAffordances && tick.Affordances)
        {
            for (const auto& affordance : *tick.Affordances)
            {
                if (affordance.Handle == engineTarget) continue;

                std::vector<LabelRow> rows{};
                rows.push_back(LabelRow{ std::string{ NameOf(affordance.Function) },
                    ToImColor(k_Affordance) });
                rows.push_back(LabelRow{ "Behaviors: " + JoinBehaviors(affordance), k_InfoColor });
                rows.push_back(LabelRow{ std::string{ "Activation: " } +
                    AffordanceFormater::ActivationToString(affordance.Activation), k_InfoColor });
                rows.push_back(LabelRow{ std::format("d: {:.1f} wu", affordance.DistanceToPlayer), k_MutedColor });

                canvas.Add(affordance.Position, std::move(rows), ToImColor(k_Affordance),
                    TeamColorOf(context, affordance.Handle), 0.0f, affordance.Handle);
            }
        }
    }
}