export module Common.Math.Type;

import std;

export namespace Common::Math::Type
{
    // Two-component vector.
    // note: also the engine vector inside the generated datum and tag structs (packed floats). Do not add members, the layout is fixed.
    struct Vec2
    {
        float X{};
        float Y{};
    };

    // Three-component vector.
    // note: also the engine vector inside the generated datum and tag structs (packed floats). Do not add members, the layout is fixed.
    struct Vec3
    {
        float X{};
        float Y{};
        float Z{};
    };

    // Four-component vector. Also used for quaternions.
    // note: also the engine vector inside the generated datum and tag structs (packed floats). Do not add members, the layout is fixed.
    struct Vec4
    {
        float X{};
        float Y{};
        float Z{};
        float W{};
    };

    static_assert(sizeof(Vec2) == 8 && std::is_trivially_copyable_v<Vec2> && std::is_standard_layout_v<Vec2>);
    static_assert(sizeof(Vec3) == 12 && std::is_trivially_copyable_v<Vec3> && std::is_standard_layout_v<Vec3>);
    static_assert(sizeof(Vec4) == 16 && std::is_trivially_copyable_v<Vec4> && std::is_standard_layout_v<Vec4>);

    // Triangle of a mesh with its surface flags and material.
    struct Triangle
    {
        Vec3 A{};
        Vec3 B{};
        Vec3 C{};

        std::uint8_t SurfaceFlags{ 0 };
        std::int16_t Material{ -1 };
    };

    // Node of a model skeleton.
    // The indices link to the parent, the next sibling and the first child, with -1 for none.
    struct Node
    {
        std::uint32_t Name{};
        std::int16_t ParentIndex{ -1 };
        std::int16_t NextSiblingIndex{ -1 };
        std::int16_t FirstChildIndex{ -1 };

        // Local-space transform of the T-pose.
        Vec3 DefaultTranslation{};
        Vec4 DefaultRotation{};
        float InverseScale{ 1.0f };

        bool DoesNotAnimate{ false };
    };

    // Up axis of the world. The engine is Z-up.
    inline constexpr Vec3 k_WorldUp{ 0.0f, 0.0f, 1.0f };

    // Full turn and quarter turn in radians.
    inline constexpr float k_TwoPi{ 6.28318530718f };
    inline constexpr float k_HalfPi{ 1.57079632679f };

    // Largest float, the start value of a running minimum.
    inline constexpr float k_FloatMax{ (std::numeric_limits<float>::max)() };

    // Row-major 4x4 matrix.
    struct Matrix : std::array<float, 16> {};
}