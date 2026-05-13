#include "pch.h"

// Header.
#include "State_MapMode.h"

// Systems.
#include "Core/Systems/Core_System.h"

// Debug.
#include "Core/Systems/Interface/System_Debug.h"

bool State_MapMode::HasMode(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Modes.count(tagName) > 0;
}

const ModeObject* State_MapMode::GetMode(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_Modes.find(tagName);
    return it != m_Modes.end() ? &it->second : nullptr;
}

void State_MapMode::AddMode(const std::string& tagName, ModeObject data)
{
    //g_pSystem->Debug->Log("[MapModeState] INFO:" 
    //    " Added: %s", tagName.c_str());

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Modes.emplace(tagName, std::move(data));
}

void State_MapMode::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Modes.clear();
}