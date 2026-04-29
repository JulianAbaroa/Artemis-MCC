#include "pch.h"
#include "Core/States/Domain/Interactable/State_Interactable.h"

std::vector<AIInteractable> State_Interactable::GetInteractables() const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Interactables;
}

void State_Interactable::SetInteractables(std::vector<AIInteractable> interactables)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Interactables = std::move(interactables);
}

bool State_Interactable::HasVehiData(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return !m_VehicleData.empty();
}

const VehicleData* State_Interactable::GetVehiData(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_VehicleData.find(tagName);
    if (it == m_VehicleData.end())
    {
        return nullptr;
    }
    return &it->second;
}

void State_Interactable::AddVehiData(const std::string& tagName, VehicleData data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_VehicleData[tagName] = std::move(data);
}

void State_Interactable::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Interactables.clear();
}