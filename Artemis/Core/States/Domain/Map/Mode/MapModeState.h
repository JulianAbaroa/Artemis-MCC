#pragma once

#include "Generated/Mode/ModeObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class MapModeState
{
public:
    bool HasMode(const std::string& tagName) const;
    const ModeObject* GetMode(const std::string& tagName) const;
    void AddMode(const std::string& tagName, ModeObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, ModeObject> m_Modes;
    mutable std::mutex m_Mutex;
};