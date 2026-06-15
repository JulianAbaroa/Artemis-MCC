#pragma once

#include <cstdint>
#include <string>

struct VitalityLayout;

template <typename TObject> class State_MapColl; struct CollObject;
template <typename TObject> class State_MapHlmt; struct HlmtObject;
class State_VitalityBuilder;
class System_Logs;

struct Sys_VitalityBuilder_Deps
{
    State_MapColl<CollObject>& State_MapColl;
    State_MapHlmt<HlmtObject>& State_MapHlmt;
    State_VitalityBuilder& State_VitalityBuilder;
    System_Logs& System_Logs;
};

class System_VitalityBuilder
{
public:
    System_VitalityBuilder(Sys_VitalityBuilder_Deps deps) : m_Deps(deps) {}
    ~System_VitalityBuilder() = default;

    void BuildForMap();

    void Cleanup();

private:
    Sys_VitalityBuilder_Deps m_Deps;

    const uint32_t k_FlagKillsObject = (1u << 0);       // "Kills Object"
    const uint32_t k_FlagKillsObjectNoSolo = (1u << 10);// "Kills Object (No Player Solo)"
    const uint32_t k_FlagHeadshot = (1u << 4);          // "Headshot"

    VitalityLayout BuildLayout(
        const HlmtObject& hlmt, const CollObject* coll) const;
};