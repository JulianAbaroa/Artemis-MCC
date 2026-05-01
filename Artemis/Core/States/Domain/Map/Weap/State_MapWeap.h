#pragma once

#include "Generated/Weap/WeapObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class State_MapWeap
{
public:
    bool HasWeap(const std::string& tagName) const;
    const WeapObject* GetWeap(const std::string& tagName) const;
    void AddWeap(const std::string& tagName, WeapObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, WeapObject> m_Weaps;
    mutable std::mutex m_Mutex;

    void LogRawData(const std::string& tagName, const WeapObject& weap);
};