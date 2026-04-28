#pragma once

// Converts a fully-deserialized SbspObject into a SbspGeometry that is
// suitable for AI navigation and reinforcement learning observations.

#include "Core/Common/Types/Domain/Map/SbspGeometry.h"
#include "Generated/Sbsp/SbspObject.h"

class SbspGeometryBuilder
{
public:
    // Takes a fully-deserialized SbspObject (all std::vector<> fields already 
    // populated by GroupDescriptor) and returns a self-contained SbspGeometry. 
    // The SbspObject can be discarded afterwards if memory is a concern.
    SbspGeometry BuildGeometry(const SbspObject& sbsp);

private:
    void BuildWorldBounds(const SbspObject& sbsp, SbspGeometry& out);
    void BuildPortals(const SbspObject& sbsp, SbspGeometry& out);
    void BuildClusters(const SbspObject& sbsp, SbspGeometry& out);
    void BuildClusterAdjacency(const SbspObject& sbsp, SbspGeometry& out);
    void BuildInstancedGeometry(const SbspObject& sbsp, SbspGeometry& out);
    void BuildCollisionMaterials(const SbspObject& sbsp, SbspGeometry& out);
    void BuildMarkers(const SbspObject& sbsp, SbspGeometry& out);

    // --- Helpers ---

    SbspVec3 MakeVec3(const Vec3& v) { return { v.X, v.Y, v.Z }; }

    SbspVec4 MakeVec4(const Vec4& v) { return { v.X, v.Y, v.Z, v.W }; }

    SbspVec3 RangeFCenter(const RangeF& x, const RangeF& y, const RangeF& z)
    { 
        return 
        { 
            (x.Min + x.Max) * 0.5f, (y.Min + y.Max) * 0.5f, (z.Min + z.Max) * 0.5f 
        }; 
    }
};