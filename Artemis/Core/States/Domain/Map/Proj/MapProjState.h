#pragma once

#include "Generated/Proj/ProjObject.h"
#include <unordered_map>
#include <string>
#include <mutex>

class MapProjState
{
public:
    bool HasProj(const std::string& tagName) const;
    const ProjObject* GetProj(const std::string& tagName) const;
    void AddProj(const std::string& tagName, ProjObject data);

    void Cleanup();

private:
    std::unordered_map<std::string, ProjObject> m_Projs;
    mutable std::mutex m_Mutex;
};