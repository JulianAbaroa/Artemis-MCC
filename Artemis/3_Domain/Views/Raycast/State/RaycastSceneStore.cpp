module Views.Raycast.State;
import :Scene;

import Views.Raycast.Type;

namespace
{
    using Views::Raycast::Type::RaycastScene;
    using Views::Raycast::Type::ScenePartsCache;
}

namespace Views::Raycast::State
{
    auto RaycastSceneStore::Scene() -> RaycastScene&
    {
        this->DropIfStale();

        return m_Scene;
    }

    auto RaycastSceneStore::Parts() -> ScenePartsCache&
    {
        this->DropIfStale();

        return m_Parts;
    }

    auto RaycastSceneStore::Cleanup() -> void
    {
        m_IsStale.store(true);
    }

    auto RaycastSceneStore::DropIfStale() -> void
    {
        if (!m_IsStale.exchange(false)) return;

        m_Scene = RaycastScene{};
        m_Parts = ScenePartsCache{};
    }
}