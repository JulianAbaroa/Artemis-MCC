export module Blam.Layout.Action.Type:Bits;

import std;

export namespace Blam::Layout::Action::Type
{
    // A button with a hold lane: its down bit and its held bit in the lanes of the action (0x5df80).
    struct HoldLane
    {
        std::uint8_t DownBit{ 0 };
        std::uint8_t HeldBit{ 0 };
    };

    // Lanes the local client fills: use, grab and enter (E, event 3: byte +0x42 is 0x10 down and 0x30 held), events 0x10, 2 (switch
    // weapon held), 5, 0x3E and 0x37.
    inline constexpr std::array<HoldLane, 6> k_HoldLanes
    {
        HoldLane{ .DownBit = 20, .HeldBit = 21 },
        HoldLane{ .DownBit = 4, .HeldBit = 5 },
        HoldLane{ .DownBit = 8, .HeldBit = 9 },
        HoldLane{ .DownBit = 12, .HeldBit = 13 },
        HoldLane{ .DownBit = 16, .HeldBit = 17 },
        HoldLane{ .DownBit = 27, .HeldBit = 29 },
    };

    // Down bit of use, grab and enter (E), the first hold lane.
    inline constexpr std::uint32_t k_UseLane{ 1u << k_HoldLanes[0].DownBit };

    // Button bit that tells the engine the zoom level of the action is absent, so the unit keeps its own (A17 sec. 2: unit_control
    // copies the zoom level every tick unless this bit is set). The local client never sets it.
    inline constexpr std::uint64_t k_ActionKeepZoomButton{ 1ull << 25 };

    // Zoom level of a unit that is not zoomed.
    inline constexpr std::int8_t k_NoZoomLevel{ -1 };
}