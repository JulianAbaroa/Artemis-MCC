#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

struct Classified;
struct LiveObject;
struct VitalityLayout;
struct ObjectVitality;
struct DamageSectionTable;

class State_Classifier;
class State_ObjectTable;
class State_DamageSections;
class State_VitalityBuilder;
class State_Vitality;
class System_Logs;

struct Sys_Vitality_Deps
{
    State_Classifier& State_Classifier;
    State_ObjectTable& State_ObjectTable;
    State_DamageSections& State_DamageSections;
    State_VitalityBuilder& State_VitalityBuilder;
    State_Vitality& State_Vitality;
    System_Logs& System_Logs;
};

class System_Vitality
{
public:
    System_Vitality(Sys_Vitality_Deps deps) : m_Deps(deps) {
    }
    ~System_Vitality() = default;

    void Update();

    void Cleanup();

private:
    Sys_Vitality_Deps m_Deps;

    static bool HasVitality(const Classified& classified);

    ObjectVitality MakeVitality(
        const LiveObject& object, const VitalityLayout& layout,
        const DamageSectionTable& damage) const;
};