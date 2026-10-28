module;

#include "External/imgui/imgui.h"

module Viewer.Hud.System;
import :VehicleLabels;

import Common.Math.Type;
import Viewer.Options.Type;
import Viewer.Style.Type;
import Viewer.Hud.Type;
import std;

namespace
{
    using Viewer::Options::Type::Flag;
    using Viewer::Hud::Type::LabelRow;

    using Viewer::Style::Type::k_Affordance;
    using Viewer::Hud::Type::k_FreeSeatColor;
    using Viewer::Hud::Type::k_TakenSeatColor;
    using Viewer::Hud::Type::k_MutedColor;

    constexpr ImU32 k_HijackerColor{ IM_COL32(230, 170, 90, 255) };
}

namespace Viewer::Hud::System
{
    auto VehicleLabels::Collect(LabelCanvas& canvas, const LabelContext& context) -> void
    {
        const auto& tick = context.Frame;
        const auto& options = canvas.GetOptions();

        if (!options.IsEnabled(Flag::VehicleSeatMarkers) || !tick.Affordances) return;

        const bool showHijackers = options.IsEnabled(Flag::VehicleHijackerSlots);

        for (const auto& affordance : *tick.Affordances)
        {
            for (const auto& seat : affordance.Seats)
            {
                if (seat.IsHijackerSlot && !showHijackers) continue;

                const auto& position = seat.SeatWorldPosition;
                if (position.X == 0.0f && position.Y == 0.0f && position.Z == 0.0f) continue;

                const ImU32 color = seat.IsHijackerSlot ? k_HijackerColor
                    : (seat.IsOccupied ? k_TakenSeatColor : k_FreeSeatColor);

                std::vector<LabelRow> rows{};
                rows.push_back(LabelRow{ seat.SeatName, color });
                rows.push_back(LabelRow{ seat.IsOccupied ? "occupied" : "free", color });
                rows.push_back(LabelRow{ context.HasSelf
                    ? std::format("d: {:.1f} wu", seat.DistanceToPlayer) : std::string{ "d: n/a" }, k_MutedColor });

                canvas.Add(position, std::move(rows), ToImColor(k_Affordance),
                    TeamColorOf(context, affordance.Handle), affordance.Handle == context.Selected ? 1.0f : 0.0f);
            }
        }
    }
}