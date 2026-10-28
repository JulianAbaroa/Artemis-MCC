module Viewer.Collidable.System;

import std;

namespace Viewer::Collidable::System
{
    auto CollidableSceneService::Update(bool isViewerActive) -> void
    {
        if (!isViewerActive)
        {
            m_CollidableStore.Publish({});
            return;
        }

        auto scene{ m_CollidableService.BuildScene() };
        if (scene) m_CollidableStore.Publish(std::move(*scene));
    }

    auto CollidableSceneService::Cleanup() -> void
    {
        m_CollidableStore.Cleanup();
    }
}