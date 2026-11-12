export module Engine.Base.MapVariant.State:MapVariant;

import Engine.Base.MapVariant.Type;
import std;

export namespace Engine::Base::MapVariant::State
{
    // The active map variant and the one the start carried (Q98, D266).
    // note: Thread-safe, and data only. Written by the match setup service (MCC and game threads), read by the Workbench. The snapshot it
    // hands out is immutable, and the game thread never reads a disk: the block is read before the request is made.
    class MapVariantStore
    {
    private:
        using ActiveMapVariant = Engine::Base::MapVariant::Type::ActiveMapVariant;

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

        // return: The map variant the start carried before the active one was written over it, or null before the first start. It is what
        // a request for none brings back.
        auto GetBase() const -> std::shared_ptr<const std::vector<std::byte>>;
        auto SetBase(std::shared_ptr<const std::vector<std::byte>> base) -> void;

    private:
        mutable std::mutex m_Mutex{};
        std::shared_ptr<const ActiveMapVariant> m_Active{};
        std::shared_ptr<const std::vector<std::byte>> m_Base{};
    };
}