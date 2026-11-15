export module Views.Raycast.State:Scene;

import Views.Raycast.Type;
import std;

namespace
{
    using Views::Raycast::Type::FixedPartsCache;
    using Views::Raycast::Type::RaycastScene;
}

export namespace Views::Raycast::State
{
    // The ray scene all agents share and the parts of the fixed objects it was gathered from, kept between ticks (D299).
    // note: Scene and Parts run on the update thread. Cleanup runs on the lifecycle thread, so it only marks the store, and the next
    // call of either empties both: the hierarchies of the meshes point into the collision models of the map, so nothing of an old
    // map may be used.
    class RaycastSceneStore
    {
    public:
        RaycastSceneStore() = default;
        ~RaycastSceneStore() = default;

        RaycastSceneStore(const RaycastSceneStore&) = delete;
        auto operator=(const RaycastSceneStore&) -> RaycastSceneStore& = delete;

        // return: The scene, to arm and cast.
        auto Scene() -> RaycastScene&;

        // return: The parts of the fixed objects of the last gather.
        auto Parts() -> FixedPartsCache&;

        // Drops the scene and the parts on the next call.
        auto Cleanup() -> void;

    private:
        RaycastScene m_Scene{};
        FixedPartsCache m_Parts{};
        std::atomic<bool> m_IsStale{ false };

        // Empties the store if a cleanup marked it.
        auto DropIfStale() -> void;
    };
}