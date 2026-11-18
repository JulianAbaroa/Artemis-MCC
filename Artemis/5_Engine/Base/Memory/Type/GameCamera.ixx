export module Engine.Base.Memory.Type:GameCamera;

import Common.Math.Type;
import std;

export namespace Engine::Base::Memory::Type::GameCamera
{
    // Floats of s_render_projection (A21 sec. 3): the render projection the game builds for each view.
    inline constexpr std::size_t k_ProjectionFloatCount{ 0x30 };

    // Index of each member in the floats of s_render_projection.
    // note: The view_to_world block is [0x0D] scale, then right, up, minus forward and position. The projection is row convention (the same
    // as the Viewer), m23 is -1.
    inline constexpr std::size_t k_ViewToWorldRight{ 0x0E };
    inline constexpr std::size_t k_ViewToWorldUp{ 0x11 };
    inline constexpr std::size_t k_ViewToWorldBack{ 0x14 };
    inline constexpr std::size_t k_ViewToWorldPosition{ 0x17 };
    inline constexpr std::size_t k_ProjectionM00{ 0x1E };
    inline constexpr std::size_t k_ProjectionM11{ 0x23 };
    inline constexpr std::size_t k_ProjectionM20{ 0x26 };
    inline constexpr std::size_t k_ProjectionM21{ 0x27 };

    // Largest error of the length of an axis, and of the dot of two axes, for the frame to count as orthonormal.
    inline constexpr float k_AxisTolerance{ 1e-3f };

    // The camera the game rendered the frame with, read from s_render_projection.
    // note: M00 and M11 are the focal values of the game projection (1 / tan of the half field of view), M20 and M21 its off-center terms.
    struct GameCamera
    {
        Common::Math::Type::Vec3 Position{};
        Common::Math::Type::Vec3 Right{};
        Common::Math::Type::Vec3 Up{};
        Common::Math::Type::Vec3 Forward{};
        float M00{};
        float M11{};
        float M20{};
        float M21{};
    };
}