#pragma once

#include "Core/Types/Domain/Domains/Environment/ModeGeometry.h"
#include "Generated/Mode/ModeObject.h"

class System_ModeGeometryBuilder
{
public:
    ModeGeometry BuildGeometry(const ModeObject& mode);

private:
    void BuildMarkerGroups(const ModeObject& mode, ModeGeometry& out);
    void BuildNodes(const ModeObject& mode, ModeGeometry& out);
    void BuildBounds(const ModeObject& mode, ModeGeometry& out);

    ModeVec3 MakeVec3(const Vec3& v) { return { v.X, v.Y, v.Z }; }
    ModeVec4 MakeVec4(const Vec4& v) { return { v.X, v.Y, v.Z, v.W }; }
};