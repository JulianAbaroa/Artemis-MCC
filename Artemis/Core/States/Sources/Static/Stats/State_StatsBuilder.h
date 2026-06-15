#pragma once

#pragma once

#include "Core/Types/Sources/Static/Stats/Vehi.h"
#include "Core/Types/Sources/Static/Stats/Weap.h"
#include "Core/Types/Sources/Static/Stats/Proj.h"

#include <unordered_map>
#include <cassert>
#include <atomic>

using Vehis = std::unordered_map<std::string, Vehi>;
using Weaps = std::unordered_map<std::string, Weap>;
using Projs = std::unordered_map<std::string, Proj>;

class State_StatsBuilder
{
public:
    // Vehi.
    bool HasVehi(const std::string& tagName) const;
    const Vehi* GetVehi(const std::string& tagName) const;
    void AddVehi(const std::string& tagName, Vehi data);

    // Weap.
    bool HasWeap(const std::string& tagName) const;
    const Weap* GetWeap(const std::string& tagName) const;
    void AddWeap(const std::string& tagName, Weap data);

    // Proj.
    bool HasProj(const std::string& tagName) const;
    const Proj* GetProj(const std::string& tagName) const;
    void AddProj(const std::string& tagName, Proj data);

    void Freeze()
    {
        m_Frozen.store(true, std::memory_order_release);
    }

    void Cleanup();

private:
    Vehis m_Vehis;
    Weaps m_Weaps;
    Projs m_Projs;

    std::atomic<bool> m_Frozen{ false };
};