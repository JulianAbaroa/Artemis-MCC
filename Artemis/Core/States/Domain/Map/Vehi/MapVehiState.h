#pragma once

#include "Generated/Vehi/VehiObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class MapVehiState
{
public:
    bool HasVehi(const std::string& tagName) const;
    const VehiObject* GetVehi(const std::string& tagName) const;
    void AddVehi(const std::string& tagName, VehiObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, VehiObject> m_Vehis;
    mutable std::mutex m_Mutex;
};