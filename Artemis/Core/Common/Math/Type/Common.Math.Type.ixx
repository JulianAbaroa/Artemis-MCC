export module Common.Math.Type;

import std;

export namespace Common::Math::Type
{
    // Two-component vector.
    struct Vec2
    {
        float X{};
        float Y{};
    };

    // Three-component vector.
    struct Vec3
    {
        float X{};
        float Y{};
        float Z{};
    };

    // Four-component vector. Also used for quaternions.
    struct Vec4
    {
        float X{};
        float Y{};
        float Z{};
        float W{};
    };

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
}