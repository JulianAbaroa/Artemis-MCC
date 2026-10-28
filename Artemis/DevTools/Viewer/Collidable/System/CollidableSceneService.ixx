export module Viewer.Collidable.System;

import Environment.Collidable.System;
import Viewer.Collidable.State;
import std;

export namespace Viewer::Collidable::System
{
    // Publishes the scene of collidables for the viewer, built apart from the pipeline (D224).
    class CollidableSceneService
    {
    private:
        using CollidableService = Environment::Collidable::System::CollidableService;

        using CollidableStore = Viewer::Collidable::State::CollidableStore;

    public:
        CollidableSceneService(CollidableService& collidableService, CollidableStore& collidableStore) :
            m_CollidableService(collidableService), m_CollidableStore(collidableStore) {}
        ~CollidableSceneService() = default;

        // Publishes the collision meshes of every live object, or an empty scene when the free camera is off.
        // note: Debug only. The collidables are not part of the Tick (D23, D84). Runs on the AI thread after the tick (D223).
        auto Update(bool isViewerActive) -> void;

        // Empties the scene.
        auto Cleanup() -> void;

    private:
        CollidableService& m_CollidableService;
        CollidableStore& m_CollidableStore;
    };
}