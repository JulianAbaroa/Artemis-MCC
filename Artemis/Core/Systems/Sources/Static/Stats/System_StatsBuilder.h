#pragma once

#include <string>

class State_MapReader;
template <typename TObject> class State_MapProj; struct ProjObject;
template <typename TObject> class State_MapVehi; struct VehiObject;
template <typename TObject> class State_MapWeap; struct WeapObject;
class State_StatsBuilder;
class System_ProjBuilder;
class System_VehiBuilder;
class System_WeapBuilder;
class System_Logs;

struct Sys_StatsBuilder_Deps
{
    State_MapReader& State_MapReader;
    State_MapProj<ProjObject>& State_MapProj;
    State_MapVehi<VehiObject>& State_MapVehi;
    State_MapWeap<WeapObject>& State_MapWeap;
    State_StatsBuilder& State_StatsBuilder;
    System_ProjBuilder& System_ProjBuilder;
    System_VehiBuilder& System_VehiBuilder;
    System_WeapBuilder& System_WeapBuilder;
    System_Logs& System_Logs;
};

class System_StatsBuilder
{
public:
    System_StatsBuilder(Sys_StatsBuilder_Deps deps) : m_Deps(deps) {}
    ~System_StatsBuilder() = default;

    void BuildForMap();

    void Cleanup();

private:
    Sys_StatsBuilder_Deps m_Deps;

    bool BuildVehi(const std::string& tagName);
    bool BuildWeap(const std::string& tagName);
    bool BuildProj(const std::string& tagName);
};