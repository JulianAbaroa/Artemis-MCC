#pragma once

#include "Core/Types/Domain/Domains/Interactable/InteractableTypes.h"
#include "Core/Types/Domain/Domains/Interactable/VehicleData.h"
#include "Core/Types/Domain/Domains/Interactable/EquipmentData.h"
#include "Core/Types/Domain/Domains/Interactable/WeaponData.h"
#include "Core/Types/Domain/Domains/Interactable/ProjectileData.h"
#include "Core/Types/Domain/Domains/Interactable/ControlDeviceData.h"
#include <vector>
#include <mutex>

class State_Interactable
{
public:
    std::vector<AIInteractable> GetInteractables() const;
    void SetInteractables(std::vector<AIInteractable> interactables);

    // VehicleData.
    bool HasVehiData(const std::string& tagName) const;
    const VehicleData* GetVehiData(const std::string& tagName) const;
    void AddVehiData(const std::string& tagName, VehicleData data);

    // EquipmentData.
    bool HasEquipmentData(const std::string& tagName) const;
    const EquipmentData* GetEquipmentData(const std::string& tagName) const;
    void AddEquipmentData(const std::string& tagName, EquipmentData data);

    // WeaponData.
    bool HasWeaponData(const std::string& tagName) const;
    const WeaponData* GetWeaponData(const std::string& tagName) const;
    void AddWeaponData(const std::string& tagName, WeaponData data);

    // ProjectileData.
    bool HasProjectileData(const std::string& tagName) const;
    const ProjectileData* GetProjectileData(const std::string& tagName) const;
    void AddProjectileData(const std::string& tagName, ProjectileData data);

    // ControlDeviceData.
    bool HasControlDeviceData(const std::string& tagName) const;
    const ControlDeviceData* GetControlDeviceData(const std::string& tagName) const;
    void AddControlDeviceData(const std::string& tagName, ControlDeviceData data);

    void Cleanup();

private:
    std::vector<AIInteractable> m_Interactables;
    std::unordered_map<std::string, VehicleData> m_VehicleData;
    std::unordered_map<std::string, EquipmentData> m_EquipmentData;
    std::unordered_map<std::string, WeaponData> m_WeaponData;
    std::unordered_map<std::string, ProjectileData> m_ProjectileData;
    std::unordered_map<std::string, ControlDeviceData> m_ControlDeviceData;
    mutable std::mutex m_Mutex;
};