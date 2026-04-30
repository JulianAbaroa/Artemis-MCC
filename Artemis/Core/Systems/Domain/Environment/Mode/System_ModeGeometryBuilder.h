#pragma once

struct ModeGeometry;
struct ModeObject;
struct ModeVec4;
struct ModeVec3;
struct Vec4;
struct Vec3;

class System_ModeGeometryBuilder
{
public:
    ModeGeometry BuildGeometry(const ModeObject& mode);

private:
    void BuildMarkerGroups(const ModeObject& mode, ModeGeometry& out);
    void BuildNodes(const ModeObject& mode, ModeGeometry& out);
    void BuildBounds(const ModeObject& mode, ModeGeometry& out);

    ModeVec3 MakeVec3(const Vec3& v);
    ModeVec4 MakeVec4(const Vec4& v);
};