module Resolved.Definitions.System;
import :Vehi;

import Resolved.Definitions.Type;

namespace
{
    using Resolved::Definitions::Type::Vehi::Seat;
    using Resolved::Definitions::Type::Vehi::SeatKind;
}

namespace Resolved::Definitions::System
{
    auto VehiBuilder::Build(const VehiObject& vehi) -> Vehi
    {
        Vehi out{};

        out.Base = m_ObjectBuilder.Build(vehi.TagName, vehi.Data, vehi.MultiplayerObject);

        out.Seats.reserve(vehi.Seats.size());

        for (const auto& source : vehi.Seats)
        {
            Seat seat
            {
                .Flags = source.Flags,
                .Kind = source.AiSeatType <= static_cast<std::uint16_t>(SeatKind::Driver)
                    ? static_cast<SeatKind>(source.AiSeatType) : SeatKind::Invalid,
                .SeatMarkerName = source.SeatMarkerName,
                .EntryMarkerName = source.EntryMarkerSName,
                .BoardingSeatIndex = source.BoardingSeatIndex,
                .EntryRadius = source.EntryRadius,
                .EntryMarkerConeAngle = source.EntryMarkerConeAngle,
                .EntryMarkerFacingAngle = source.EntryMarkerFacingAngle,
                .MaximumRelativeVelocity = source.MaximumRelativeVelocity,
            };

            seat.AdditionalBoardingSeats.reserve(source.AdditionalBoardingSeats.size());

            for (const auto& boarded : source.AdditionalBoardingSeats)
            {
                seat.AdditionalBoardingSeats.push_back(boarded.SeatIndex);
            }

            out.Seats.push_back(std::move(seat));
        }

        return out;
    }
}