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
}