module Engine.Base.Preset.State;
import :Preset;
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

    auto PresetStore::SetBase(std::shared_ptr<const PresetBase> base) -> void
    {
        std::lock_guard<std::mutex> lock{ m_Mutex };
        m_Base = std::move(base);
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
}