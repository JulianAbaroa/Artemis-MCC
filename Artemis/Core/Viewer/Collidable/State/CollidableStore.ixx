export module Viewer.Collidable.State;

import Common.State.Type;
import Environment.Collidable.Type;
import std;

namespace
{
    using Environment::Collidable::Type::Collidables;
}

export namespace Viewer::Collidable::State
{
    // Holds the scene of collidables the viewer draws and selects from.
    // note: Published by the CollidableSceneService on the AI thread and emptied on map cleanup (game thread). Read by the render thread.
    class CollidableStore : public Common::State::Type::Snapshot<Collidables>
    {
    };
}