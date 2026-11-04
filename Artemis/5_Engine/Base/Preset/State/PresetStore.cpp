module Engine.Base.Preset.State;
import :Preset;
import Engine.Base.Preset.Type;
import std;

namespace Engine::Base::Preset::State
{
    auto PresetStore::GetActive() const -> std::shared_ptr<const CompiledPreset>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Active;
    }

    auto PresetStore::GetActiveName() const -> std::string
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Active != nullptr ? m_Active->Name : std::string{};
    }

    auto PresetStore::SetActive(std::shared_ptr<const CompiledPreset> preset) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Active = std::move(preset);
    }

    auto PresetStore::GetCatalog() const -> std::shared_ptr<const PresetCatalog>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Catalog;
    }

    auto PresetStore::SetCatalog(std::shared_ptr<const PresetCatalog> catalog) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Catalog = std::move(catalog);
    }

    auto PresetStore::GetBase() const -> std::shared_ptr<const PresetBase>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Base;
    }

    auto PresetStore::GetBaseCatalog() const -> std::shared_ptr<const PresetCatalog>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_BaseCatalog;
    }

    auto PresetStore::SetBase(std::shared_ptr<const PresetBase> base, std::shared_ptr<const PresetCatalog> catalog) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Base = std::move(base);
        m_BaseCatalog = m_Base != nullptr ? std::move(catalog) : nullptr;
    }

    auto PresetStore::BeginRequest() -> std::uint64_t
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return ++m_LatestRequestId;
    }

    auto PresetStore::HasPending() const -> bool
    {
        return m_HasPending.load(std::memory_order_acquire);
    }

    auto PresetStore::SetPending(PendingRequest request) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Pending = std::move(request);
        m_HasPending.store(true, std::memory_order_release);
    }

    auto PresetStore::TakePending() -> std::optional<PendingRequest>
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        std::optional<PendingRequest> request{ std::move(m_Pending) };
        m_Pending.reset();
        m_HasPending.store(false, std::memory_order_release);
        return request;
    }

    auto PresetStore::ClearPending() -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Pending.reset();
        m_HasPending.store(false, std::memory_order_release);
    }

    auto PresetStore::GetResult() const -> PresetResult
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        return m_Result;
    }

    auto PresetStore::SetResult(PresetResult result) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Result = std::move(result);
    }

    auto PresetStore::SetResultOf(std::uint64_t requestId, PresetResult result) -> bool
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        if (requestId != m_LatestRequestId) return false;

        m_Result = std::move(result);
        return true;
    }
}