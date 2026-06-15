#include "pch.h"

#include "System_StatsBuilder.h"

#include "Core/Types/Sources/MapReader/MapTypes.h"
#include "Core/Types/Sources/MapReader/MapMagics.h"

#include "Core/States/Sources/MapReader/State_MapReader.h"
#include "Core/States/Sources/MapReader/Tags/State_MapProj.h"
#include "Core/States/Sources/MapReader/Tags/State_MapVehi.h"
#include "Core/States/Sources/MapReader/Tags/State_MapWeap.h"
#include "Core/States/Sources/Static/Stats/State_StatsBuilder.h"

#include "Proj/System_ProjBuilder.h"
#include "Vehi/System_VehiBuilder.h"
#include "Weap/System_WeapBuilder.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

void System_StatsBuilder::BuildForMap()
{
    auto& sta_reader = m_Deps.State_MapReader;

    int32_t proj{}, vehi{}, weap{};

    const int32_t tagCount = static_cast<int32_t>(
        sta_reader.GetTagsSize());

    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& entry = sta_reader.GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const std::string tagName = sta_reader.GetTagName(i);
        if (tagName.empty()) continue;

        const uint32_t magic = sta_reader.GetGroupMagic(
            entry.TagGroupIndex);

        if (magic == MapMagics::k_ProjMagic)
        {
            if (!this->BuildProj(tagName)) continue;
            ++proj;
        }
        else if (magic == MapMagics::k_VehiMagic)
        {
            if (!this->BuildVehi(tagName)) continue;
            ++vehi;
        }
        else if (magic == MapMagics::k_WeapMagic)
        {
            if (!this->BuildWeap(tagName)) continue;
            ++weap;
        }
    }

    m_Deps.State_StatsBuilder.Freeze();

    m_Deps.System_Logs.Log("[StatsBuilder] INFO: Stats built."
        " Proj: %d | Vehi: %d | Weap: %d", proj, vehi, weap);
}

bool System_StatsBuilder::BuildProj(const std::string& tagName)
{
    const ProjObject* proj = m_Deps.State_MapProj.Get(tagName);
    if (!proj)
    {
        m_Deps.System_Logs.Log("[StatsBuilder] WARNING:"
            " Proj tag found in table but not loaded: %s", 
            tagName.c_str());
        return false;
    }

    Proj data = m_Deps.System_ProjBuilder.Build(*proj);
    m_Deps.State_StatsBuilder.AddProj(tagName, std::move(data));
    return true;
}

bool System_StatsBuilder::BuildVehi(const std::string& tagName)
{
    const VehiObject* vehi = m_Deps.State_MapVehi.Get(tagName);
    if (!vehi)
    {
        m_Deps.System_Logs.Log("[StatsBuilder] WARNING:"
            " Vehi tag found in table but not loaded: %s", 
            tagName.c_str());
        return false;
    }

    Vehi data = m_Deps.System_VehiBuilder.Build(*vehi);
    m_Deps.State_StatsBuilder.AddVehi(tagName, std::move(data));
    return true;
}

bool System_StatsBuilder::BuildWeap(const std::string& tagName)
{
    const WeapObject* weap = m_Deps.State_MapWeap.Get(tagName);
    if (!weap)
    {
        m_Deps.System_Logs.Log("[StatsBuilder] WARNING:"
            " Weap tag found in table but not loaded: %s", 
            tagName.c_str());
        return false;
    }

    Weap data = m_Deps.System_WeapBuilder.Build(*weap);
    m_Deps.State_StatsBuilder.AddWeap(tagName, std::move(data));
    return true;
}

void System_StatsBuilder::Cleanup()
{
    m_Deps.State_StatsBuilder.Cleanup();

    m_Deps.System_Logs.Log("[StatsBuilder] INFO:"
        " Cleanup completed.");
}