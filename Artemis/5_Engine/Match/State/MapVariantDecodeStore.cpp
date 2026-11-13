module Engine.Match.State;
import :MapVariantDecode;

import Engine.Match.Type;
import std;

namespace Engine::Match::State
{
    auto MapVariantDecodeStore::Request(std::vector<std::byte> file) -> std::uint64_t
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Request = MapVariantDecodeRequest{ .Ticket = ++m_LastTicket, .File = std::move(file) };
        m_Outcome.reset();
        m_HasRequest.store(true);
        return m_LastTicket;
    }

    auto MapVariantDecodeStore::HasRequest() const -> bool
    {
        return m_HasRequest.load();
    }

    auto MapVariantDecodeStore::TakeRequest() -> std::optional<MapVariantDecodeRequest>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_HasRequest.store(false);
        return std::exchange(m_Request, std::nullopt);
    }

    auto MapVariantDecodeStore::Publish(MapVariantDecodeOutcome outcome) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };

        // A newer request replaced the one this answers: the result is stale.
        if (outcome.Ticket != m_LastTicket) return;

        m_Outcome = std::move(outcome);
    }

    auto MapVariantDecodeStore::TakeOutcome(std::uint64_t ticket) -> std::optional<MapVariantDecodeOutcome>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };

        if (!m_Outcome || m_Outcome->Ticket != ticket) return std::nullopt;

        return std::exchange(m_Outcome, std::nullopt);
    }
}