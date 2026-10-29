module Perception.Classifier.State;
import :TagClassifications;

namespace
{
    // Key of a tag: group and tag name, since two groups can share a tag name.
    auto KeyOf(std::string_view fourCC, std::string_view tagName) -> std::string
    {
        std::string key{};
        key.reserve(fourCC.size() + tagName.size() + 1);
        key.append(fourCC).append(1, '|').append(tagName);
        return key;
    }
}

namespace Perception::Classifier::State
{
    auto TagClassificationsStore::Find(std::string_view fourCC, std::string_view tagName) -> const TagClassification*
    {
        if (m_IsStale.exchange(false))
        {
            m_Classifications.clear();
            m_IsSummaryLogged = false;
        }

        const auto it{ m_Classifications.find(KeyOf(fourCC, tagName)) };
        return it == m_Classifications.end() ? nullptr : &it->second;
    }

    auto TagClassificationsStore::Add(std::string_view fourCC, std::string_view tagName, TagClassification classification) -> void
    {
        m_Classifications.insert_or_assign(KeyOf(fourCC, tagName), classification);
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