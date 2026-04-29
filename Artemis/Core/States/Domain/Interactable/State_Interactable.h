#pragma once

#include "Core/Types/Domain/Domains/Interactable/InteractableTypes.h"
#include "Core/Types/Domain/Domains/Interactable/VehicleData.h"
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

    void Cleanup();

private:
    std::vector<AIInteractable> m_Interactables;
    std::unordered_map<std::string, VehicleData> m_VehicleData;
    mutable std::mutex m_Mutex;
};