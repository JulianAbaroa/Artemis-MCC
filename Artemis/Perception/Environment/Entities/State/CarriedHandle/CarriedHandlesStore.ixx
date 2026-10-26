export module Environment.Entities.State:CarriedHandles;

import std;

export namespace Environment::Entities::State
{
    // Handles of the objects a biped carried since the map loaded.
    // note: A Fixed or Phased item that was dropped cannot be picked up again (D80), so IsPickable needs this history.
    // Contains and Add run on the update thread. Cleanup runs on the lifecycle thread, so it only marks the store, and the next call empties it.
    class CarriedHandlesStore
    {
    public:
        CarriedHandlesStore() = default;
        ~CarriedHandlesStore() = default;

        CarriedHandlesStore(const CarriedHandlesStore&) = delete;
        auto operator=(const CarriedHandlesStore&) -> CarriedHandlesStore& = delete;

        // Checks whether the object was carried since the last cleanup.
        auto Contains(std::uint32_t handle) -> bool;

        // Keeps the handle of a carried object.
        auto Add(std::uint32_t handle) -> void;

        // Drops every handle on the next call.
        auto Cleanup() -> void;

    private:
        std::unordered_set<std::uint32_t> m_Handles{};
        std::atomic<bool> m_IsStale{ false };

        // Empties the store if a cleanup marked it.
        auto DropIfStale() -> void;
    };
}