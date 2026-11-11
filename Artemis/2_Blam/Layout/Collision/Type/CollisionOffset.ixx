export module Blam.Layout.Collision.Type:Offset;

import std;

export namespace Blam::Layout::Collision::Type::Offset
{
    // Result kind (uint32): 0 nothing, 1 structure, 2 special test (flag bit 1), 3 instanced geometry, 4 object.
    constexpr std::size_t k_ResultKind{ 0x00 };

    // Fraction of the vector where the ray hit (float, 0 to 1). The distance is this times the length of the vector.
    constexpr std::size_t k_Fraction{ 0x04 };

    // Point of the impact (three floats).
    constexpr std::size_t k_Point{ 0x08 };

    // Global material index of the surface (int16, -1 or 0xFFFF when it has none).
    constexpr std::size_t k_Material{ 0x28 };

    // Handle of the object that was hit (uint32, 0xFFFFFFFF when none).
    constexpr std::size_t k_ObjectHandle{ 0x40 };

    // Whether the back face of the surface was hit (uint8).
    constexpr std::size_t k_IsBackFace{ 0x5C };

    // Flags of the hit surface (uint8): the same bits as the flags of the collision surface of the map.
    constexpr std::size_t k_SurfaceFlags{ 0x5D };

    // Two more bytes of the surface the walk of the BSP copies, meaning not measured.
    constexpr std::size_t k_SurfaceByteA{ 0x5E };
    constexpr std::size_t k_SurfaceByteB{ 0x62 };
}