#pragma once

#include "Generated/Mach/MachObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class State_MapMach
{
public:
    bool HasMach(const std::string& tagName) const;
    const MachObject* GetMach(const std::string& tagName) const;
    void AddMach(const std::string& tagName, MachObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, MachObject> m_Machs;
    mutable std::mutex m_Mutex;

    void LogRawData(const std::string& tagName, const MachObject& mach);
};