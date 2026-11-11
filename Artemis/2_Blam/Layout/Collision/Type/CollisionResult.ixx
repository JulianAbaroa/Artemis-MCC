export module Blam.Layout.Collision.Type:Result;

import :Constant;
import :Offset;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Blam::Layout::Collision::Type::Constant::k_ResultSize;

    // return: The little endian integer of the bytes at an offset.
    constexpr auto ReadBytes(std::span<const std::uint8_t, k_ResultSize> bytes, std::size_t offset, std::size_t size) -> std::uint32_t
    {
        std::uint32_t value{ 0 };
        for (std::size_t index{ 0 }; index < size; ++index)
        {
            value |= static_cast<std::uint32_t>(bytes[offset + index]) << (8 * index);
        }
        return value;
    }
}

export namespace Blam::Layout::Collision::Type::Result
{
    // What the engine reports at the offset k_Kind of its result.
    enum class ResultKind : std::uint32_t
    {
        None = 0,
        Structure = 1,
        Special = 2,
        InstancedGeometry = 3,
        Object = 4,
    };

    // The part of the result of collision_test_vector that Artemis compares with its own rays (A20 sec. 1.4).
    struct CollisionResult
    {
        // The value the function returned: whether a surface or an object was hit.
        bool IsHit{ false };
        ResultKind Kind{ ResultKind::None };

        // Fraction of the vector. 1 when nothing was hit.
        float Fraction{ 1.0f };
        std::array<float, 3> Point{};

        // Global material index, -1 when the surface has none.
        std::int16_t Material{ -1 };
        std::uint32_t ObjectHandle{ k_NoHandle };

        bool IsBackFace{ false };
        std::uint8_t SurfaceFlags{ 0 };
        std::uint8_t SurfaceByteA{ 0 };
        std::uint8_t SurfaceByteB{ 0 };
    };

    // return: The fields of the 0x70 bytes the engine wrote, with the value the function returned.
    constexpr auto Decode(bool isHit, std::span<const std::uint8_t, k_ResultSize> bytes) -> CollisionResult
    {
        CollisionResult result{};

        result.IsHit = isHit;
        result.Kind = static_cast<ResultKind>(ReadBytes(bytes, Offset::k_ResultKind, 4));
        result.Fraction = std::bit_cast<float>(ReadBytes(bytes, Offset::k_Fraction, 4));

        for (std::size_t axis{ 0 }; axis < result.Point.size(); ++axis)
        {
            result.Point[axis] = std::bit_cast<float>(ReadBytes(bytes, Offset::k_Point + (4 * axis), 4));
        }

        result.Material = static_cast<std::int16_t>(ReadBytes(bytes, Offset::k_Material, 2));
        result.ObjectHandle = ReadBytes(bytes, Offset::k_ObjectHandle, 4);
        result.IsBackFace = bytes[Offset::k_IsBackFace] != 0;
        result.SurfaceFlags = bytes[Offset::k_SurfaceFlags];
        result.SurfaceByteA = bytes[Offset::k_SurfaceByteA];
        result.SurfaceByteB = bytes[Offset::k_SurfaceByteB];

        return result;
    }
}