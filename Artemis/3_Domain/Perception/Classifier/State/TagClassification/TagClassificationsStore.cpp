module Perception.Classifier.State;
import :TagClassifications;

namespace Perception::Classifier::State
{
    auto TagClassificationsStore::Refresh() -> void
    {
        if (!m_IsStale.exchange(false)) return;

        m_Classifications.clear();
        m_IsSummaryLogged = false;
    }

    auto TagClassificationsStore::Find(std::uint32_t datumIndex) const -> const TagClassification*
    {
        // note: A map change that no Refresh has seen yet must not answer with the tags of the old map.
        if (m_IsStale.load(std::memory_order_relaxed)) return nullptr;

        const auto it{ m_Classifications.find(datumIndex) };
        return it == m_Classifications.end() ? nullptr : &it->second;
    }

    auto TagClassificationsStore::Add(std::uint32_t datumIndex, TagClassification classification) -> void
    {
        m_Classifications.insert_or_assign(datumIndex, classification);
    }

    auto TagClassificationsStore::GetAll() const -> const TagClassifications&
    {
        return m_Classifications;
    }

    auto TagClassificationsStore::IsSummaryLogged() const -> bool
    {
        return m_IsSummaryLogged;
    }

    auto TagClassificationsStore::SetSummaryLogged(bool value) -> void
    {
        m_IsSummaryLogged = value;
    }

    auto TagClassificationsStore::Cleanup() -> void
    {
        m_IsStale.store(true);
    }
}