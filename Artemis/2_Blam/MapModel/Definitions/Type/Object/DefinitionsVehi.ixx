export module Blam.MapModel.Definitions.Type:Vehi;

import :Object;

import std;

export namespace Blam::MapModel::Definitions::Type::Vehi
{
    // Seat type the AI uses, same ids as the tag.
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

    // Place where a unit can sit in a vehicle, in the order of the tag.
    // note: The seat elements of the vehicle datum (block 0x560) follow the same order.
    struct Seat
    {
        // Bit 2 driver, bit 3 gunner, bit 10 boarding seat, bit 14 invalid for player.
        std::uint32_t Flags{};

        SeatKind Kind{ SeatKind::Invalid };

        // Marker group of the render model where the unit sits.
        std::uint32_t SeatMarkerName{};

        // Marker group of the render model a unit must be close to, to enter.
        std::uint32_t EntryMarkerName{};

        // Seat that this one boards, -1 for none.
        std::int16_t BoardingSeatIndex{ -1 };

        // Extra seats ejected when this one boards.
        std::vector<std::int16_t> AdditionalBoardingSeats{};

        // Distance to the entry marker a unit must be within.
        float EntryRadius{};

        // Angle from the forward of the entry marker the unit must be within.
        float EntryMarkerConeAngle{};

        // Angle from the facing of the unit the entry marker must be within.
        float EntryMarkerFacingAngle{};

        float MaximumRelativeVelocity{};
    };

    // Holds the vehicle definition.
    struct Vehi
    {
        Object::Object Base{};

        std::vector<Seat> Seats{};
    };
}