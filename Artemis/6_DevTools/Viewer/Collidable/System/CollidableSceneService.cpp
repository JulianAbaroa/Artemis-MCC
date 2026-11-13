module DevTools.Viewer.Collidable.System;

import DevTools.Viewer.Collidable.Type;
import std;

namespace
{
    using DevTools::Viewer::Collidable::Type::CollidableScene;
}

namespace DevTools::Viewer::Collidable::System
{
    auto CollidableSceneService::Update(const std::shared_ptr<const Tick>& tick, bool isViewerActive) -> void
    {
        CollidableScene scene{ .Tick = tick };

        if (isViewerActive)
        {
            auto objects{ m_CollidableService.BuildScene() };
            if (objects) scene.Objects = std::move(*objects);
        }

        m_CollidableStore.Publish(std::move(scene));
    }

    auto CollidableSceneService::Cleanup() -> void
    {
        m_CollidableStore.Cleanup();
    }
}