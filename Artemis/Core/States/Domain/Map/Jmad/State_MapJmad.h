#pragma once

#include "Generated/Jmad/JmadObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class State_MapJmad
{
public:
    bool HasJmad(const std::string& tagName) const;
    const JmadObject* GetJmad(const std::string& tagName) const;
    void AddJmad(const std::string& tagName, JmadObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, JmadObject> m_Jmads;
    mutable std::mutex m_Mutex;
};