#pragma once

#include "Generated/Sldt/SldtObject.h"

#include <unordered_map>
#include <string>
#include <mutex>

class State_MapSldt
{
public:
    bool HasSldt(const std::string& tagName) const;
    const SldtObject* GetSldt(const std::string& tagName) const;
    void AddSldt(const std::string& tagName, SldtObject zone);
    void Cleanup();

private:
    std::unordered_map<std::string, SldtObject> m_Sldts;
    mutable std::mutex m_Mutex;
};