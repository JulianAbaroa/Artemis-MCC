export module Viewer.Camera.Type;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec2;
    using Common::Math::Type::Vec3;
    // 4x4 matrix stored row by row.
    using Common::Math::Type::Matrix;
}

export namespace Viewer::Camera::Type
{
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
        Custom,

        Count
    };

    // How the camera is driven while the viewer is active.
    // Free: the viewer takes the keyboard and the mouse and the camera flies on its own.
    // Game: the viewer takes nothing, the game keeps its controls and the camera sits on the local player view.
    enum class Mode : std::uint8_t
    {
        Free,
        Game
    };

    // What the Free mode camera does with the selected object.
    // Off: flies on its own. FirstPerson: sits on the view of the object. Orbit: turns around the object at a distance.
    // note: Without a selected object the camera flies as in Off and takes the object as soon as one is selected.
    enum class Follow : std::uint8_t
    {
        Off,
        FirstPerson,
        Orbit
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