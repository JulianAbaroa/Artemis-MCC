#pragma once

struct Vec3;
struct Vec4;
struct ModeVec3;
struct ModeVec4;
struct ModeObject;
struct ModeGeometry;

class System_Logs;

struct Sys_ModeBuilder_Deps
{
    System_Logs& System_Logs;
};

class System_ModeBuilder
{
public:
    System_ModeBuilder(Sys_ModeBuilder_Deps deps) : m_Deps(deps) {}
    ~System_ModeBuilder() = default;

    ModeGeometry Build(const ModeObject& mode);

private:
    Sys_ModeBuilder_Deps m_Deps;

    void BuildMarkerGroups(const ModeObject& mode, ModeGeometry& out);
    void BuildNodes(const ModeObject& mode, ModeGeometry& out);
    void BuildBounds(const ModeObject& mode, ModeGeometry& out);

    ModeVec3 MakeVec3(const Vec3& v);
    ModeVec4 MakeVec4(const Vec4& v);
};