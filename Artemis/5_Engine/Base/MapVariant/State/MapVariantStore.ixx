export module Engine.Base.MapVariant.State:MapVariant;

import Engine.Base.MapVariant.Type;
import std;

export namespace Engine::Base::MapVariant::State
{
    // The active map variant and the result of the last load or request (Q98, D266).
    // note: Thread-safe, and data only. Written by the bench (bench thread) and the start hook (MCC thread), read by the start hook and the
    // Workbench. The snapshot it hands out is immutable, and the game thread never reads a disk: the block is read before it is set.
    class MapVariantStore
    {
    private:
        using ActiveMapVariant = Engine::Base::MapVariant::Type::ActiveMapVariant;
        using MapVariantResult = Engine::Base::MapVariant::Type::MapVariantResult;

    public:
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

        auto GetResult() const -> MapVariantResult;
        auto SetResult(MapVariantResult result) -> void;

    private:
        mutable std::mutex m_Mutex{};
        std::shared_ptr<const ActiveMapVariant> m_Active{};
        MapVariantResult m_Result{};
    };
}