export module Template.Object.Type:Block;

import std;

// Variable blocks of the object datum. Each one is found through a descriptor { u16 size, u16 offset from the
// datum start } stored in the fixed part, and its position changes with the tag of the object.
// note: Written by hand until the block descriptors are generated from the XML templates (pending item 4).
export namespace Template::Object::Type::Block
{
    // Seat state of a vehicle: one element per seat of the vehicle tag, in the order of the tag.
    // The hijacker seats are elements too.
    namespace Seat
    {
        constexpr std::size_t k_Descriptor{ 0x560 };

        // Bytes of one seat element: { f32 current, f32 target, u32 3 }.
        constexpr std::size_t k_ElementSize{ 0x0C };

        // Smoothed value that ramps towards the target every tick.
        constexpr std::size_t k_Current{ 0x00 };

        // 1.0 = free, 0.0 = occupied (or being hijacked, for a hijacker seat).
        constexpr std::size_t k_Target{ 0x04 };
    }

    // Multiplayer data of an object: zone shape, team and teleport settings.
    // note: Present in every object type (0x88 bytes). Which fields are meaningful depends on the tag.
    namespace Multiplayer
    {
        constexpr std::size_t k_Descriptor{ 0x190 };

        // Zone shape. Length is only used by the box shape.
        constexpr std::size_t k_Radius{ 0x00 };
        constexpr std::size_t k_Length{ 0x04 };
        constexpr std::size_t k_Top{ 0x08 };
        constexpr std::size_t k_Bottom{ 0x0C };

        // 0 = none, 2 = cylinder, 3 = box. The value 1 was not tested.
        constexpr std::size_t k_ShapeKind{ 0x10 };

        // Teleport channel and allowed objects (teleport crates).
        constexpr std::size_t k_Channel{ 0x15 };
        constexpr std::size_t k_Allowed{ 0x16 };

        // Team of the flag, the bomb, the spawn points, the zones and the boundaries.
        constexpr std::size_t k_Team{ 0x19 };
    }
}