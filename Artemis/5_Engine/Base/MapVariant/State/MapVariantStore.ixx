export module Engine.Base.MapVariant.State:MapVariant;

import Engine.Base.MapVariant.Type;
import std;

export namespace Engine::Base::MapVariant::State
{
    // The active map variant, the one the start carried, the request waiting for a round break and the result of the last load or request
    // (Q98, D266).
    // note: Thread-safe, and data only. Written by the bench and the bridge (their threads), the start hook (MCC thread) and the tick hook (game
    // thread), read by the hooks and the Workbench. The snapshot it hands out is immutable, and the game thread never reads a disk: the block is read before it is set.
    class MapVariantStore
    {
    private:
        using ActiveMapVariant = Engine::Base::MapVariant::Type::ActiveMapVariant;
        using MapVariantResult = Engine::Base::MapVariant::Type::MapVariantResult;

    public:
        // A map variant requested during a match that waits for a round break. A null map variant asks for none: the one the match loaded
        // with comes back. Id is the one BeginRequest gave: the result of a request is only written while it is the latest one.
        struct PendingRequest
        {
            std::uint64_t Id{ 0 };
            std::string Name{};
            std::shared_ptr<const ActiveMapVariant> MapVariant{};
        };

        MapVariantStore() = default;
        ~MapVariantStore() = default;

        MapVariantStore(const MapVariantStore&) = delete;
        auto operator=(const MapVariantStore&) -> MapVariantStore& = delete;

        // return: The active map variant, or null when there is none.
        auto GetActive() const -> std::shared_ptr<const ActiveMapVariant>;

        // return: The name of the active map variant, empty when there is none.
        auto GetActiveName() const -> std::string;

        // Makes a map variant the active one, or none with null. The next load writes it.
        auto SetActive(std::shared_ptr<const ActiveMapVariant> mapVariant) -> void;

        // return: The map variant the start carried before the active one was written over it, or null before the first start. It is what
        // a request for none brings back.
        auto GetBase() const -> std::shared_ptr<const std::vector<std::byte>>;
        auto SetBase(std::shared_ptr<const std::vector<std::byte>> base) -> void;

        // return: A new request id. It is the latest one until the next call.
        auto BeginRequest() -> std::uint64_t;

        // return: Whether a request waits for a round break. Lock free: the game thread asks it every frame.
        // True while the id is still the latest one: no newer request, bench choice or clearing came after it.
        auto IsCurrent(std::uint64_t requestId) const -> bool;
        auto HasPending() const -> bool;

        // Leaves a request to serve at a round break, replacing the one waiting.
        auto SetPending(PendingRequest request) -> void;

        // Reads the request waiting and clears it in one step.
        // return: The request, or none.
        auto TakePending() -> std::optional<PendingRequest>;

        // Drops the request waiting.
        auto ClearPending() -> void;

        auto GetResult() const -> MapVariantResult;
        auto SetResult(MapVariantResult result) -> void;

        // Sets the result only if the request is still the latest one, so the end of an old request never hides a newer one.
        // return: Whether it was set.
        auto SetResultOf(std::uint64_t requestId, MapVariantResult result) -> bool;

    private:
        mutable std::mutex m_Mutex{};
        std::shared_ptr<const ActiveMapVariant> m_Active{};
        std::shared_ptr<const std::vector<std::byte>> m_Base{};
        std::optional<PendingRequest> m_Pending{};
        std::uint64_t m_LatestRequestId{ 0 };
        MapVariantResult m_Result{};

        std::atomic<bool> m_HasPending{ false };
    };
}