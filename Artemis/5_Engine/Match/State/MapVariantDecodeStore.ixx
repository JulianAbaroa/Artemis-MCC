export module Engine.Match.State:MapVariantDecode;

import Engine.Match.Type;
import std;

export namespace Engine::Match::State
{
    // The request to decode a .mvar and its result (Q106). The bench or the UI asks from its thread, the lifecycle hook answers on the MCC
    // thread.
    // note: Thread-safe. One request waits at a time: a new one replaces it, and the older ticket is never answered.
    class MapVariantDecodeStore
    {
    private:
        using MapVariantDecodeOutcome = Engine::Match::Type::MapVariantDecodeOutcome;
        using MapVariantDecodeRequest = Engine::Match::Type::MapVariantDecodeRequest;

    public:
        MapVariantDecodeStore() = default;
        ~MapVariantDecodeStore() = default;

        MapVariantDecodeStore(const MapVariantDecodeStore&) = delete;
        auto operator=(const MapVariantDecodeStore&) -> MapVariantDecodeStore& = delete;

        // Asks for the decode of a file. Drops the result of the last one.
        // param file: The bytes of the whole .mvar.
        // return: The ticket the result carries.
        auto Request(std::vector<std::byte> file) -> std::uint64_t;

        // return: Whether a request waits. Cheap: the hook asks it every frame.
        auto HasRequest() const -> bool;

        // return: The waiting request, nothing if none waits.
        auto TakeRequest() -> std::optional<MapVariantDecodeRequest>;

        // Keeps the result of a request until it is taken.
        auto Publish(MapVariantDecodeOutcome outcome) -> void;

        // return: The result of that ticket, nothing while it is not there. It is given once.
        auto TakeOutcome(std::uint64_t ticket) -> std::optional<MapVariantDecodeOutcome>;

    private:
        mutable std::mutex m_Mutex{};
        std::optional<MapVariantDecodeRequest> m_Request{};
        std::optional<MapVariantDecodeOutcome> m_Outcome{};
        std::uint64_t m_LastTicket{ 0 };
        std::atomic<bool> m_HasRequest{ false };
    };
}