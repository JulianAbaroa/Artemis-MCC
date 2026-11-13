export module DevTools.Viewer.Collidable.State;

import Common.State.Type;
import DevTools.Viewer.Collidable.Type;
import std;

namespace
{
    using DevTools::Viewer::Collidable::Type::CollidableScene;
}

export namespace DevTools::Viewer::Collidable::State
{
    // Holds the scene the viewer draws and selects from: the collidables with the Tick they belong to (D294).
    // note: Published by the CollidableSceneService on the AI thread and emptied on map cleanup (game thread). Read by the render thread.
    class CollidableStore : public Common::State::Type::Snapshot<CollidableScene>
    {
    };
}