#include "pch.h"

// Header.
#include "State_MapPhmo.h"

bool State_MapPhmo::HasPhmo(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Phmos.count(tagName) > 0;
}

const PhmoObject* State_MapPhmo::GetPhmo(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Phmos.find(tagName);
    return it != m_Phmos.end() ? &it->second : nullptr;
}

void State_MapPhmo::AddPhmo(const std::string& tagName, PhmoObject data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Phmos.emplace(tagName, std::move(data));
}

size_t State_MapPhmo::GetSize() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Phmos.size();
}

std::vector<std::string> State_MapPhmo::GetTagNames()
{
    std::lock_guard<std::mutex> lock(m_Mutex);

    std::vector<std::string> names;
    names.reserve(m_Phmos.size());

    for (const auto& [name, object] : m_Phmos) names.push_back(name);

    return names;
}

void State_MapPhmo::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Phmos.clear();
}