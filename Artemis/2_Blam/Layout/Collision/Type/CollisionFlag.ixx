export module Blam.Layout.Collision.Type:Flag;

import std;

export namespace Blam::Layout::Collision::Type::Flag
{
    // Bit 0 of the flags of collision_test_vector: the collision BSP of the structure.
    constexpr std::uint64_t k_Structure{ 1ull << 0 };

    // Bit 3: the instanced geometry of the clusters the ray crosses.
    constexpr std::uint64_t k_InstancedGeometry{ 1ull << 3 };

    // Bit 32: the objects of the clusters the ray crosses. The types come from bits 33 to 46 or from k_AllObjectTypes.
    constexpr std::uint64_t k_Objects{ 1ull << 32 };

    // Bit 47: every object type.
    constexpr std::uint64_t k_AllObjectTypes{ 1ull << 47 };

    // Bit 23: the engine adds the mask bit 0x1000 (a class of objects the object test skips) unless this bit is set (0x12d750). It does
    // not touch surfaces. Setting it keeps that class in the test: bit 0 of the mask is always set and nothing clears it (A20 sec. 1.3.1).
    constexpr std::uint64_t k_KeepObjectClass1000{ 1ull << 23 };

    // Bits 6 to 22: each one narrows the test. Bits 11 to 16 are the walk mask of the BSP (11 and 12 pick a face, with none of them both
    // are tested, 14 skips invisible surfaces, 15 breakable ones, 16 stops at the first impact), bits 6 and 10 skip instance
    // placements, bit 17 moves the point to a valid location, and the rest feed the mask of 0x12d750 that skips classes of objects.
    // A ray that must meet every surface and both faces keeps all of them clear (A20 sec. 1.3).
    constexpr std::uint64_t k_NarrowingBits{ ((1ull << 23) - 1) & ~((1ull << 6) - 1) };

    // Structure and instanced geometry, every surface, both faces (all walk bits clear): what the static part of the rays of Artemis is compared with.
    constexpr std::uint64_t k_Static{ k_Structure | k_InstancedGeometry | k_KeepObjectClass1000 };

    // k_Static and every object type.
    constexpr std::uint64_t k_StaticAndObjects{ k_Static | k_Objects | k_AllObjectTypes };

    static_assert((k_Static & k_NarrowingBits) == 0 && (k_StaticAndObjects & k_NarrowingBits) == 0);
}