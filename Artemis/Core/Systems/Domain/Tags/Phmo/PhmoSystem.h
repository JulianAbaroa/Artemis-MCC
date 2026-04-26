#pragma once

//#include "Core/Common/Types/Domain/Map/PhmoTypes.h"

class PhmoSystem
{
public:
    // Called ~16ms from AIThread. 
    void UpdatePhmos();

    //void Cleanup();

private:
    // Helpers.
    void CrossProduct(const float a[3], const float b[3], float out[3]);

    void RotateByOrientation(const float modelPoint[3], const float forward[3],
        const float up[3], float outWorld[3]);

    void TransformHalfExtents(const float modelHE[3], const float forward[3],
        const float up[3], float outWorldHE[3]);

    //void ExpandWithPill(const PhmoPill& pill, const float position[3],
    //    const float forward[3], const float up[3], float unionMin[3],
    //    float unionMax[3], RBSnapshot& snap);

    //void ExpandWithSphere(const PhmoSphere& sphere, const float position[3],
    //    const float forward[3], const float up[3], float unionMin[3],
    //    float unionMax[3], RBSnapshot& snap);

    //void ExpandWithBox(const PhmoBox& box, const float position[3],
    //    const float forward[3], const float up[3], float unionMin[3],
    //    float unionMax[3], RBSnapshot& snap);

    //void ExpandWithTriangle(const PhmoTriangle& tri, const float position[3],
    //    const float forward[3], const float up[3], float unionMin[3],
    //    float unionMax[3], RBSnapshot& snap);

    //void ExpandWithMultiSphere(const PhmoMultiSphere& ms, const float position[3],
    //    const float forward[3], const float up[3], float unionMin[3],
    //    float unionMax[3], RBSnapshot& snap);
};