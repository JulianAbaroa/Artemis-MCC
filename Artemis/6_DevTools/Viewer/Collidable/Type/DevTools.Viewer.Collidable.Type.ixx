export module DevTools.Viewer.Collidable.Type;

import Perception.Collidable.Type;
import Bridge.Codec.Tick.Type;
import std;

namespace
{
    using Perception::Collidable::Type::Collidables;
}

export namespace DevTools::Viewer::Collidable::Type
{
    // What the viewer draws in one frame: the collidables and the Tick they were built after, published together.
    // note: The scene is built after the Tick is published (D223). Read apart, the Tick store can be one tick ahead of the meshes and
    // the camera, the labels and the meshes disagree for the frames in between (D294).
    struct CollidableScene
    {
        std::shared_ptr<const Bridge::Codec::Tick::Type::Tick> Tick{};
        // Empty while the free camera is off.
        Collidables Objects{};
    };

    // return: The collidables of the scene, sharing its lifetime, or null without a scene.
    inline auto ShareObjects(const std::shared_ptr<const CollidableScene>& scene) -> std::shared_ptr<const Collidables>
    {
        if (!scene) return nullptr;
        return std::shared_ptr<const Collidables>{ scene, &scene->Objects };
    }
}