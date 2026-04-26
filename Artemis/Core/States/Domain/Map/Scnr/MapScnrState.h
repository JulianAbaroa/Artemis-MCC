#pragma once

#include "Generated/Scnr/ScnrObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class MapScnrState
{
public:
    bool HasScnr(const std::string& tagName) const;
    const ScnrObject* GetScnr(const std::string& tagName) const;
    void AddScnr(const std::string& tagName, ScnrObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, ScnrObject> m_Scnrs;
    mutable std::mutex m_Mutex;
};