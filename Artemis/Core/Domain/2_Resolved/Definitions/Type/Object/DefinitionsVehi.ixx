export module Resolved.Definitions.Type:Vehi;

import :Object;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Vec4;
}

export namespace Resolved::Definitions::Type::Vehi
{
    enum class SeatKind : std::uint8_t
    {
        None = 0x00,
        Passenger = 0x01,
        Gunner = 0x02,
        SmallCargo = 0x03,
        LargeCargo = 0x04,
        Driver = 0x05,

        Invalid = 0xFF,
    };

    // Places where a unit can sit in a vehicle.
    // note: Planned for semantization. No builder fills it yet.
    struct Seat
    {
        std::uint32_t SeatMarkerNameId{};
        std::int8_t NodeIndex{};
        SeatKind Kind{ SeatKind::Invalid };
        Vec3 LocalTranslation{};
        Vec4 LocalRotation{};
        float EntryRadius{};
    };

    // Holds the vehicle definition.
    struct Vehi
    {
        Object::Object Base{};

        std::vector<Seat> Seats{};
    };
}