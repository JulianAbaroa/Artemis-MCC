module;

#include "External/imgui/imgui.h"

module DevTools.Viewer.Hud.System;
import :ObsLabels;

import Common.Engine.Type;
import Common.Math.Type;
import Common.Math.System;
import Common.Team.Type;
import Views.View.Type;
import Views.View.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Style.Type;
import DevTools.Viewer.Hud.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Math::Type::Vec3;
    using Common::Team::Type::Team;
    using Views::View::Type::ObsTargets;
    using Views::View::Type::View;
    using Views::View::System::ObsTargetsOf;
    using Views::View::System::RivalTeamOf;
    using DevTools::Viewer::Style::Type::Color;
    using DevTools::Viewer::Style::Type::ColorOfTeam;
    using DevTools::Viewer::Hud::Type::LabelRow;

    constexpr float k_ObsPriority{ 0.5f };

    // Adds the label of one arrow: its length, in the color of the arrow, at its tip.
    auto AddDistance(DevTools::Viewer::Hud::System::LabelCanvas& canvas, const Vec3& from, const Vec3& to, const Color& color) -> void
    {
        const ImU32 packed{ DevTools::Viewer::Hud::System::ToImColor(color) };

        std::vector<LabelRow> rows{};
        rows.push_back(LabelRow{ std::format("{:.1f} wu", Common::Math::System::Distance(from, to)), packed });

        canvas.Add(to, std::move(rows), packed, packed, k_ObsPriority, k_NoHandle);
    }
}

namespace DevTools::Viewer::Hud::System
{
    auto ObsLabels::Collect(LabelCanvas& canvas, const LabelContext& context, const std::vector<std::size_t>& rows) -> void
    {
        const auto& tick = context.Frame;
        if (!tick.Views) return;

        for (const std::size_t row : rows)
        {
            if (row >= tick.Views->size()) continue;

            const View& view{ (*tick.Views)[row] };
            const ObsTargets targets{ ObsTargetsOf(view) };

            if (targets.HasFlag)
            {
                const Team rival{ tick.ObjectiveDetails ? RivalTeamOf(*tick.ObjectiveDetails, view.Head.Team) : Team::Neutral };
                AddDistance(canvas, targets.From, targets.Flag, ColorOfTeam(rival));
            }

            if (targets.HasStand) AddDistance(canvas, targets.From, targets.Stand, ColorOfTeam(view.Head.Team));
        }
    }
}