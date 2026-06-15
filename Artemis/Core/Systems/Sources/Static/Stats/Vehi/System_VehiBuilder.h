#pragma once

struct Vehi;
struct VehiObject;

class System_Logs;

struct Sys_VehiBuilder_Deps
{
    System_Logs& System_Logs;
};

class System_VehiBuilder
{
public:
    System_VehiBuilder(Sys_VehiBuilder_Deps deps) : m_Deps(deps) {}
    ~System_VehiBuilder() = default;

    Vehi Build(const VehiObject& vehi);

private:
    Sys_VehiBuilder_Deps m_Deps;
};