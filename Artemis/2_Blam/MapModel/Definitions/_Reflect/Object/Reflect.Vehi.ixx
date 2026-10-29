export module Blam.MapModel.Definitions.Reflect:Vehi;

import :Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Vehi::Seat;
    using Blam::MapModel::Definitions::Type::Vehi::Vehi;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Seat>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Flags", &Seat::Flags),
            MakeField("Kind", &Seat::Kind),
            MakeField("SeatMarkerName", &Seat::SeatMarkerName),
            MakeField("EntryMarkerName", &Seat::EntryMarkerName),
            MakeField("BoardingSeatIndex", &Seat::BoardingSeatIndex),
            MakeField("AdditionalBoardingSeats", &Seat::AdditionalBoardingSeats),
            MakeField("EntryRadius", &Seat::EntryRadius),
            MakeField("EntryMarkerConeAngle", &Seat::EntryMarkerConeAngle),
            MakeField("EntryMarkerFacingAngle", &Seat::EntryMarkerFacingAngle),
            MakeField("MaximumRelativeVelocity", &Seat::MaximumRelativeVelocity),
        };
    };

    template <>
    struct Fields<Vehi>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Vehi::Base),
            MakeField("Seats", &Vehi::Seats),
        };
    };
}