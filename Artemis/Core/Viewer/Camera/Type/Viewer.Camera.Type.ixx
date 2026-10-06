export module Viewer.Camera.Type;

import Common.Math.Type;
import std;

export namespace Viewer::Camera::Type
{
    using Vec2 = Common::Math::Type::Vec2;
    using Vec3 = Common::Math::Type::Vec3;

    // 4x4 matrix stored row by row.
    using Matrix = std::array<float, 16>;

    // Camera movement control. Count is the number of keys and is not a key.
    enum class Key : std::uint8_t
    {
        Forward,
        Backward,
        Left,
        Right,
        Up,
        Down,
        Fast,
        Slow,

        Count
    };

    // Perspective of the camera. FovY is the vertical field of view in radians.
    struct Lens
    {
        float FovY{ 1.0472f };
        float Near{ 0.05f };
        float Far{ 10000.0f };
    };

    // Screen rectangle the camera renders to, in pixels.
    struct Viewport
    {
        Vec2 Position{};
        Vec2 Size{};
    };

    // Half line in world space. Direction is normalized.
    struct Ray
    {
        Vec3 Origin{};
        Vec3 Direction{};
    };
}