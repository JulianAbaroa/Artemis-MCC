#include "pch.h"

#include "State_MapPlay.h"

bool State_MapPlay::HasPlay(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Plays.count(tagName) > 0;
}

const PlayObject* State_MapPlay::GetPlay(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Plays.find(tagName);
    return it != m_Plays.end() ? &it->second : nullptr;
}

void State_MapPlay::AddPlay(const std::string& tagName, PlayObject zone)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Plays[tagName] = std::move(zone);
}

const PlayObject* State_MapPlay::GetFirstPlay() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    if (m_Plays.empty()) return nullptr;
    return &m_Plays.begin()->second;
}

void State_MapPlay::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Plays.clear();
}