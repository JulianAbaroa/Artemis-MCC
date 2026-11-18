export module DevTools.Viewer.Interpolation.System;

import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Interpolation.Type;
import std;

export namespace DevTools::Viewer::Interpolation::System
{
    // Finds the pose each dynamic object is drawn with between two ticks, as the game draws it (VW.5e, D330). The one place where it is done.
    // note: Pure and stateless. It runs on the thread that draws, after the scene is chosen, and calls no engine function.
    class InterpolationService
    {
    private:
        using CollidableScene = DevTools::Viewer::Collidable::Type::CollidableScene;
        using FramePoses = DevTools::Viewer::Interpolation::Type::FramePoses;

    public:
        // Fills the poses of the frame against the scene shown.
        // param shown: The scene the frame draws (the tick the game drew, T - 1).
        // param newer: The scene of the next tick (T) if it is in the history, else null.
        // param older: The scene of the tick before the shown one (T - 2) if it is in the history, else null.
        // param fraction: The tick fraction of the clock of the game, 0 to 1.
        // note: Objects are matched by full handle. With the next tick the object is blended between the two, else it is moved on at the pace of
        // the last two. An object missing in the scene used, a step past k_InterpolationSnapDistance and the objects that stay where they are
        // (including Fixed and Phased ones, which the game does not move) are not listed. With neither scene the result is empty and inactive.
        static auto Build(const CollidableScene& shown, const std::shared_ptr<const CollidableScene>& newer,
            const std::shared_ptr<const CollidableScene>& older, float fraction, FramePoses& out) -> void;
    };
}