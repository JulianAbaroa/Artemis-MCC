export module Resolved.Stats.Type:Vehi;

import Common.Math.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Vec4 = Common::Math::Type::Vec4;
}

export namespace Resolved::Stats::Type::Vehi
{
    enum class SeatType : std::uint8_t
    {
        None = 0,
        Passenger = 1,
        Gunner = 2,
        SmallCargo = 3,
        LargeCargo = 4,
        Driver = 5,
    };

    struct Seat
    {
        std::uint32_t SeatMarkerNameId = 0;
        std::int8_t NodeIndex;
        SeatType SeatType = SeatType::None;
        Vec3 LocalTranslation{};
        Vec4 LocalRotation{};
        float EntryRadius = 0.0f;
    };

    struct Vehi
    {
        std::string TagName;
        std::vector<Seat> Seats;
    };
}