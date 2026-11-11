module Engine.Base.MapVariant.State;
import :MapVariant;

import Engine.Base.MapVariant.Type;
import std;

namespace Engine::Base::MapVariant::State
{
    auto MapVariantStore::GetActive() const -> std::shared_ptr<const ActiveMapVariant>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Active;
    }

    auto MapVariantStore::GetActiveName() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Active != nullptr ? m_Active->Name : std::string{};
    }

    auto MapVariantStore::SetActive(std::shared_ptr<const ActiveMapVariant> mapVariant) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Active = std::move(mapVariant);
    }

    auto MapVariantStore::GetBase() const -> std::shared_ptr<const std::vector<std::byte>>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Base;
    }

    auto MapVariantStore::SetBase(std::shared_ptr<const std::vector<std::byte>> base) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Base = std::move(base);
    }

    auto MapVariantStore::BeginRequest() -> std::uint64_t
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return ++m_LatestRequestId;
    }

    auto MapVariantStore::IsCurrent(std::uint64_t requestId) const -> bool
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return requestId == m_LatestRequestId;
    }

    auto MapVariantStore::HasPending() const -> bool
    {
        return m_HasPending.load(std::memory_order_acquire);
    }

    auto MapVariantStore::SetPending(PendingRequest request) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Pending = std::move(request);
        m_HasPending.store(true, std::memory_order_release);
    }

    auto MapVariantStore::TakePending() -> std::optional<PendingRequest>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        std::optional<PendingRequest> request{ std::move(m_Pending) };
        m_Pending.reset();
        m_HasPending.store(false, std::memory_order_release);
        return request;
    }

    auto MapVariantStore::ClearPending() -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Pending.reset();
        m_HasPending.store(false, std::memory_order_release);
    }

    auto MapVariantStore::GetResult() const -> MapVariantResult
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Result;
    }

    auto MapVariantStore::SetResult(MapVariantResult result) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Result = std::move(result);
    }

    auto MapVariantStore::SetResultOf(std::uint64_t requestId, MapVariantResult result) -> bool
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (requestId != m_LatestRequestId) return false;

        m_Result = std::move(result);
        return true;
    }
}