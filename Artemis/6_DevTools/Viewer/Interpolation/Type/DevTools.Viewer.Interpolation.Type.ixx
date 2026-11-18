export module DevTools.Viewer.Interpolation.Type;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::PoseSource;
    using Common::Math::Type::Vec3;
}

export namespace DevTools::Viewer::Interpolation::Type
{
    // A step of an object between two ticks longer than this (wu) is a teleport, a respawn or a change of seat, and is not blended (VW.5e, D330).
    // The choice is 2 wu per tick, far above what anything the game moves covers in 1/60 s.
    inline constexpr float k_InterpolationSnapDistance{ 2.0f };

    // How the pose an object is drawn with differs from the one of the scene shown.
    struct ObjectDelta
    {
        std::uint32_t Handle{};
        // Render position minus the position of the scene shown. Anything anchored to the object by a point moves by this.
        Vec3 Offset{};
        // Moves what is placed with the pose of the scene shown to where the render pose places it (3x4, row vectors, as the instance transforms).
        std::array<float, 12> Transform{};
    };

    // The poses the objects are drawn with in one frame, against the scene shown (VW.5e, D330).
    // note: Only objects that moved are listed. An object that is not listed is drawn as the scene shows it.
    struct FramePoses
    {
        // Changes with the fraction and the tick, so a pass that caches its upload by scene knows the poses changed. Zero when nothing is blended.
        std::uint32_t Revision{ 0 };
        // Exact: the next tick was in the history. Extrapolated: only the one before the shown. None: nothing was blended.
        PoseSource Source{ PoseSource::None };
        // Sorted by handle.
        std::vector<ObjectDelta> Objects{};

        auto IsActive() const -> bool
        {
            return Revision != 0;
        }

        // return: The delta of the object with the full handle (index and salt), or null if it is drawn as the scene shows it.
        auto Find(std::uint32_t handle) const -> const ObjectDelta*
        {
            const auto it{ std::ranges::lower_bound(Objects, handle, {}, &ObjectDelta::Handle) };
            return (it == Objects.end() || it->Handle != handle) ? nullptr : &*it;
        }

        // return: How far the object is drawn from where the scene shows it, zero if it is not listed.
        auto OffsetOf(std::uint32_t handle) const -> Vec3
        {
            const ObjectDelta* delta{ this->Find(handle) };
            return delta ? delta->Offset : Vec3{};
        }
    };
}