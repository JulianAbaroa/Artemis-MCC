#pragma once

#include "Generated/Scen/ScenObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class MapScenState
{
public:
    bool HasScen(const std::string& tagName) const;
    const ScenObject* GetScen(const std::string& tagName) const;
    void AddScen(const std::string& tagName, ScenObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, ScenObject> m_Scens;
    mutable std::mutex m_Mutex;
};