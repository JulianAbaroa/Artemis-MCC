export module DevTools.Viewer.Collidable.System;

import Perception.Collidable.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Collidable.State;
import std;

export namespace DevTools::Viewer::Collidable::System
{
    // Publishes the scene of collidables for the viewer, built apart from the pipeline (D224).
    class CollidableSceneService
    {
    private:
        using CollidableService = Perception::Collidable::System::CollidableService;
        using Tick = Bridge::Codec::Tick::Type::Tick;

        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;

    public:
        CollidableSceneService(CollidableService& collidableService, CollidableStore& collidableStore) :
            m_CollidableService(collidableService), m_CollidableStore(collidableStore) {}
        ~CollidableSceneService() = default;

        // Publishes the Tick with the collision meshes of every live object, or with none when the free camera is off.
        // param tick: The Tick just published. The viewer draws this one, not the Tick store, so meshes and Tick match (D294).
        // note: Debug only. The collidables are not part of the Tick (D23, D84). Runs on the AI thread after the tick (D223).
        auto Update(const std::shared_ptr<const Tick>& tick, bool isViewerActive) -> void;

        // Empties the scene.
        auto Cleanup() -> void;

    private:
        CollidableService& m_CollidableService;
        CollidableStore& m_CollidableStore;
    };
}