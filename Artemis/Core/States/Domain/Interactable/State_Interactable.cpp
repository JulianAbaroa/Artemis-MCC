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

bool State_Interactable::HasEquipmentData(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_EquipmentData.count(tagName) > 0;
}

const EquipmentData* State_Interactable::GetEquipmentData(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_EquipmentData.find(tagName);
    return it != m_EquipmentData.end() ? &it->second : nullptr;
}

void State_Interactable::AddEquipmentData(const std::string& tagName, EquipmentData data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_EquipmentData[tagName] = std::move(data);
}

bool State_Interactable::HasWeaponData(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_WeaponData.count(tagName) > 0;
}

const WeaponData* State_Interactable::GetWeaponData(const std::string& tagName) const
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    auto it = m_WeaponData.find(tagName);
    return it != m_WeaponData.end() ? &it->second : nullptr;
}

void State_Interactable::AddWeaponData(const std::string& tagName, WeaponData data)
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_WeaponData[tagName] = std::move(data);
}

void State_Interactable::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Interactables.clear();
    m_VehicleData.clear();
    m_EquipmentData.clear();
    m_WeaponData.clear();
}