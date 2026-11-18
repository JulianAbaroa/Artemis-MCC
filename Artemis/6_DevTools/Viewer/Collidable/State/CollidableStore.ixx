export module DevTools.Viewer.Collidable.State;

import Common.State.Type;
import DevTools.Viewer.Collidable.Type;
import std;

namespace
{
    using Common::State::Type::SnapshotHistory;
    using DevTools::Viewer::Collidable::Type::CollidableScene;
    using DevTools::Viewer::Collidable::Type::SceneTickOf;

    using DevTools::Viewer::Collidable::Type::k_SceneHistory;
}

export namespace DevTools::Viewer::Collidable::State
{
    // Holds the last scenes the viewer draws and selects from: the collidables with the Tick they belong to, keyed by the game tick (D294).
    // note: Published by the CollidableSceneService on the AI thread and emptied on map cleanup (game thread). Read by the render thread, which
    // draws the scene of the tick the game drew (the history) and may wait a few ms for it (WaitForKey, never while the AI thread needs a lock
    // to publish: the store mutex is only held to copy a pointer).
    class CollidableStore : public SnapshotHistory<CollidableScene, k_SceneHistory, SceneTickOf>
    {
    };
}