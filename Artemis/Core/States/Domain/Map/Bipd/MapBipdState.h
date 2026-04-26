#pragma once

#include "Generated/Bipd/BipdObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class MapBipdState
{
public:
    bool HasBipd(const std::string& tagName) const;
    const BipdObject* GetBipd(const std::string& tagName) const;
    void AddBipd(const std::string& tagName, BipdObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, BipdObject> m_Bipds;
    mutable std::mutex m_Mutex;
};