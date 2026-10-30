export module Blam.Runtime.Object.Type:Constant;

import std;

export namespace Blam::Runtime::Object::Type::Constant
{
    constexpr std::uint8_t k_DamageSectionStride{ 0x18 };
    constexpr std::uint16_t k_RegionBlockMaxSize{ 0x400 };

    // Bit of the placement flags (Mvar +0x00) set when the slot holds a placement.
    constexpr std::uint16_t k_PlacementInUseBit{ 0x0001 };

    // Bits of the Forge flags of a placement (Mvar +0x46). Symmetry takes two bits from k_SymmetryShift.
    // note: Phased sets both physics bits (0xC0), Fixed only 0x40.
    constexpr std::uint8_t k_NotPlacedAtStartBit{ 0x02 };
    constexpr std::uint8_t k_SymmetryShift{ 2 };
    constexpr std::uint8_t k_SymmetryMask{ 0x03 };
    constexpr std::uint8_t k_GameSpecificBit{ 0x20 };
    constexpr std::uint8_t k_FixedBit{ 0x40 };
    constexpr std::uint8_t k_PhasedBit{ 0x80 };

    // Color of a placement left on the Forge default.
    constexpr std::uint8_t k_DefaultColor{ 0xFF };

    // Bits of the action byte of a weapon: trigger held, zoomed, and the action of the weapon.
    constexpr std::uint8_t k_TriggerHeldBit{ 0x02 };
    constexpr std::uint8_t k_ZoomedBit{ 0x80 };
    constexpr std::uint8_t k_ActionMask{ 0x60 };

    // Overheated bit of the weapon flags (a 16 bit field, unlike the action byte).
    constexpr std::uint16_t k_OverheatedBit{ 0x0002 };
}