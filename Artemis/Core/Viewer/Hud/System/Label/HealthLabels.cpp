module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :HealthLabels;

import Common.Math.Type;
import Resolved.World.Type;
import Environment.Health.Type;
import Viewer.Options.State;
import Viewer.Options.Type;
import Viewer.Style.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Flag = Viewer::Options::Type::Flag;
    using AnchorSource = Resolved::World::Type::ModelLink::AnchorSource;
    using Section = Environment::Health::Type::VitalitySection;
    using SectionKind = Environment::Health::Type::VitalityKind;

    using Viewer::Style::Type::k_VitalityShield;

    constexpr std::uint32_t k_NoHandle{ 0xFFFFFFFF };
    constexpr float k_SelectedPriority{ 1.0f };

    auto TagsOf(const Section& section) -> std::string
    {
        std::string tags{};

        if (section.IsCritical) tags += 'K';
        if (section.IsHeadshot) tags += 'H';
        if (section.DestroysObject) tags += 'D';
        if (section.Kind == SectionKind::Shield) tags += 'S';

        return tags;
    }
}

namespace Viewer::Hud::System
{
    auto HealthLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = context.Options;

        if (!options.IsEnabled(Flag::HealthBars)) return;
        if (!tick.Aims || !tick.Healths) return;

        const bool selectedOnly = options.IsEnabled(Flag::HealthSelectedOnly);
        const bool showValue = options.IsEnabled(Flag::HealthBarValue);
        const bool showTags = options.IsEnabled(Flag::HealthBarTags);
        const bool showShields = options.IsEnabled(Flag::HealthShieldSections);
        const bool includeCenter = options.IsEnabled(Flag::HealthIncludeCenter);

        auto emit = [&](std::uint32_t handle)
        {
            const auto aimIt = tick.Aims->find(handle);
            const auto healthIt = tick.Healths->find(handle);
            if (aimIt == tick.Aims->end() || healthIt == tick.Healths->end()) return;

            const auto& layout = healthIt->second.Layout;

            const auto& sections = aimIt->second.Sections;
            const auto& vitalities = healthIt->second.SectionVitalities;
            const float priority = handle == context.Selected ? k_SelectedPriority : 0.0f;

            for (std::size_t i = 0; i < sections.size() && i < vitalities.size(); ++i)
            {
                const auto& aim = sections[i];
                if (!aim.Valid) continue;
                if (aim.AimSource == AnchorSource::ObjectCenter && !includeCenter) continue;

                const Section* layoutSection = (layout && i < layout->Sections.size())
                    ? &layout->Sections[i] : nullptr;

                const bool isShield = layoutSection && layoutSection->Kind == SectionKind::Shield;
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

        for (const auto& entry : *tick.Healths)
        {
            emit(entry.first);
        }
    }
}