module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :HealthLabels;

import Common.Engine.Type;
import Common.Math.Type;
import Resolved.Vitality.Type;
import Resolved.World.Type;
import Viewer.Options.Type;
import Viewer.Options.State;
import Viewer.Style.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Viewer::Options::Type::Flag;
    using Resolved::World::Type::ModelLink::AnchorSource;
    using Resolved::Vitality::Type::Vitality::Section;
    using Resolved::Vitality::Type::Vitality::VitalityKind;

    using Viewer::Style::Type::k_VitalityShield;
    using Common::Engine::Type::k_NoHandle;
    using Viewer::Hud::Type::k_SelectedPriority;

    auto TagsOf(const Section& section) -> std::string
    {
        std::string tags{};

        if (section.IsCritical) tags += 'K';
        if (section.IsHeadshot) tags += 'H';
        if (section.DestroysObject) tags += 'D';
        if (section.Kind == VitalityKind::Shield) tags += 'S';

        return tags;
    }
}

namespace Viewer::Hud::System
{
    auto HealthLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = canvas.GetOptions();

        if (!options.IsEnabled(Flag::HealthBars)) return;
        if (!tick.Aims || !tick.VitalityDetails) return;

        const bool selectedOnly = options.IsEnabled(Flag::HealthSelectedOnly);
        const bool showValue = options.IsEnabled(Flag::HealthBarValue);
        const bool showTags = options.IsEnabled(Flag::HealthBarTags);
        const bool showShields = options.IsEnabled(Flag::HealthShieldSections);
        const bool includeCenter = options.IsEnabled(Flag::HealthIncludeCenter);

        auto emit = [&](std::uint32_t handle)
        {
            const auto aimIt = tick.Aims->find(handle);
            const auto detailIt = tick.VitalityDetails->find(handle);
            if (aimIt == tick.Aims->end() || detailIt == tick.VitalityDetails->end()) return;

            const auto& layout = detailIt->second.Layout;

            const auto& sections = aimIt->second.Sections;
            const auto& vitalities = detailIt->second.Sections;
            const float priority = handle == context.Selected ? k_SelectedPriority : 0.0f;

            for (std::size_t i = 0; i < sections.size() && i < vitalities.size(); ++i)
            {
                const auto& aim = sections[i];
                if (!aim.Valid) continue;
                if (aim.AimSource == AnchorSource::ObjectCenter && !includeCenter) continue;

                const Section* layoutSection = (layout && i < layout->Sections.size())
                    ? &layout->Sections[i] : nullptr;

                const bool isShield = layoutSection && layoutSection->Kind == VitalityKind::Shield;
                if (isShield && !showShields) continue;

                const float vitality = vitalities[i];

                std::string text{};
                if (showTags && layoutSection) text = TagsOf(*layoutSection);

                if (showValue)
                {
                    if (!text.empty()) text += ' ';
                    text += std::format("{:.2f}", vitality);
                }

                const ImU32 color = isShield
                    ? ToImColor(k_VitalityShield)
                    : ToImColor(Viewer::Style::Type::ColorOfVitality(vitality));

                canvas.AddBar(aim.Position, vitality, color, std::move(text), priority);
            }
        };

        if (selectedOnly)
        {
            if (context.Selected != k_NoHandle) emit(context.Selected);
            return;
        }

        for (const auto& entry : *tick.VitalityDetails)
        {
            emit(entry.first);
        }
    }
}