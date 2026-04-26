#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Map/MapState.h"
#include "Core/States/Domain/Map/Phmo/MapPhmoState.h"
#include "Core/States/Domain/Tables/ObjectTableState.h"
#include "Core/States/Domain/Tags/Phmo/PhmoState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Map/MapSystem.h"
#include "Core/Systems/Domain/Tags/Phmo/PhmoSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include <unordered_set>

void PhmoSystem::UpdatePhmos()
{
    const auto& allObjects = g_pState->Domain->ObjectTable->GetAllObjects();

    //std::unordered_map<uint32_t, LivePhmoObject> snapshot;
    //snapshot.reserve(allObjects.size());

    //for (const auto& [handle, object] : allObjects)
    //{
    //    LivePhmoObject lop;
    //    lop.Handle = handle;

    //    const PhmoObject* pmd =
    //        g_pState->Domain->MapPhmo->GetPhmo(object.TagName);

    //    if (!pmd || pmd->RigidBodies.empty())
    //    {
    //        snapshot.emplace(handle, lop);
    //        continue;
    //    }

    //    float unionMin[3] = { FLT_MAX,  FLT_MAX,  FLT_MAX };
    //    float unionMax[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    //    float maxRadius = 0.0f;
    //    float dominantCenter[3] = {};

    //    for (const auto& rb : pmd->RigidBodies)
    //    {
    //        RBSnapshot snap;
    //        snap.NodeIndex = rb.NodeIndex;

    //        // Bounding sphere.
    //        float rotOff[3];
    //        this->RotateByOrientation(rb.BoundingSphereOffset,
    //            object.Forward, object.Up, rotOff);

    //        snap.SphereCenter[0] = object.Position[0] + rotOff[0];
    //        snap.SphereCenter[1] = object.Position[1] + rotOff[1];
    //        snap.SphereCenter[2] = object.Position[2] + rotOff[2];
    //        snap.SphereRadius = rb.BoundingSphereRadius;

    //        if (rb.BoundingSphereRadius > maxRadius)
    //        {
    //            maxRadius = rb.BoundingSphereRadius;
    //            dominantCenter[0] = snap.SphereCenter[0];
    //            dominantCenter[1] = snap.SphereCenter[1];
    //            dominantCenter[2] = snap.SphereCenter[2];
    //        }

    //        // Shape-specific AABB.
    //        int16_t idx = rb.ShapeIndex;

    //        switch (rb.ShapeType)
    //        {
    //        case PhmoShapeType::Sphere:
    //            if (idx >= 0 && (size_t)idx < pmd->Spheres.size())
    //            {
    //                this->ExpandWithSphere(pmd->Spheres[idx], object.Position, 
    //                    object.Forward, object.Up, unionMin, unionMax, snap);
    //            }

    //            break;

    //        case PhmoShapeType::Pill:
    //            if (idx >= 0 && (size_t)idx < pmd->Pills.size())
    //            {
    //                this->ExpandWithPill(pmd->Pills[idx], object.Position, 
    //                    object.Forward, object.Up, unionMin, unionMax, snap);
    //            }

    //            break;

    //        case PhmoShapeType::Box:
    //            if (idx >= 0 && (size_t)idx < pmd->Boxes.size())
    //            {
    //                this->ExpandWithBox(pmd->Boxes[idx], object.Position, 
    //                    object.Forward, object.Up, unionMin, unionMax, snap);
    //            }

    //            break;

    //        case PhmoShapeType::Triangle:
    //            if (idx >= 0 && (size_t)idx < pmd->Triangles.size())
    //            { 
    //                this->ExpandWithTriangle(pmd->Triangles[idx], object.Position, 
    //                    object.Forward, object.Up, unionMin, unionMax, snap);
    //            }

    //            break;

    //        case PhmoShapeType::Polyhedron:
    //            if (idx >= 0 && (size_t)idx < pmd->Polyhedra.size())
    //            {
    //                const PhmoPolyhedron& poly = pmd->Polyhedra[idx];

    //                const float* he = poly.AABBHalfExtents;
    //                const float* mc = poly.AABBCenter;

    //                if (he[0] > 0.0f || he[1] > 0.0f || he[2] > 0.0f)
    //                {
    //                    float rotCenter[3];
    //                    this->RotateByOrientation(mc, 
    //                        object.Forward, object.Up, rotCenter);

    //                    snap.AABBCenter[0] = object.Position[0] + rotCenter[0];
    //                    snap.AABBCenter[1] = object.Position[1] + rotCenter[1];
    //                    snap.AABBCenter[2] = object.Position[2] + rotCenter[2];

    //                    this->TransformHalfExtents(he, 
    //                        object.Forward, object.Up, snap.AABBHalfExtents);

    //                    snap.HasAABB = true;

    //                    for (int k = 0; k < 3; ++k)
    //                    {
    //                        float lo = snap.AABBCenter[k] - snap.AABBHalfExtents[k];
    //                        float hi = snap.AABBCenter[k] + snap.AABBHalfExtents[k];
    //                        if (lo < unionMin[k]) unionMin[k] = lo;
    //                        if (hi > unionMax[k]) unionMax[k] = hi;
    //                    }
    //                }
    //            }

    //            break;

    //        case PhmoShapeType::MultiSphere:
    //            if (idx >= 0 && (size_t)idx < pmd->MultiSpheres.size())
    //            {
    //                this->ExpandWithMultiSphere(pmd->MultiSpheres[idx],
    //                    object.Position, object.Forward, object.Up,
    //                    unionMin, unionMax, snap);
    //            }

    //            break;

    //        case PhmoShapeType::List:
    //        case PhmoShapeType::Mopp:
    //            for (const auto& poly : pmd->Polyhedra)
    //            {
    //                const float* he = poly.AABBHalfExtents;
    //                const float* mc = poly.AABBCenter;

    //                if (he[0] <= 0.0f && he[1] <= 0.0f && he[2] <= 0.0f) continue;

    //                float rotCenter[3];
    //                this->RotateByOrientation(mc, 
    //                    object.Forward, object.Up, rotCenter);

    //                float wc[3] = {
    //                    object.Position[0] + rotCenter[0],
    //                    object.Position[1] + rotCenter[1],
    //                    object.Position[2] + rotCenter[2],
    //                };

    //                float whe[3];
    //                this->TransformHalfExtents(he, 
    //                    object.Forward, object.Up, whe);

    //                // Update snap with largest poly.
    //                float polyVol = whe[0] * whe[1] * whe[2];
    //                float snapVol = snap.AABBHalfExtents[0] *
    //                    snap.AABBHalfExtents[1] * snap.AABBHalfExtents[2];

    //                if (!snap.HasAABB || polyVol > snapVol)
    //                {
    //                    for (int k = 0; k < 3; ++k)
    //                    {
    //                        snap.AABBCenter[k] = wc[k];
    //                        snap.AABBHalfExtents[k] = whe[k];
    //                    }

    //                    snap.HasAABB = true;
    //                }

    //                for (int k = 0; k < 3; ++k)
    //                {
    //                    if ((wc[k] - whe[k]) < unionMin[k])
    //                    {
    //                        unionMin[k] = wc[k] - whe[k];
    //                    }

    //                    if ((wc[k] + whe[k]) > unionMax[k])
    //                    {
    //                        unionMax[k] = wc[k] + whe[k];
    //                    }
    //                }
    //            }

    //            // Also expand with pills/spheres/boxes if present.
    //            for (const auto& pill : pmd->Pills)
    //            {
    //                RBSnapshot tmp{};
    //                this->ExpandWithPill(pill, object.Position, object.Forward,
    //                    object.Up, unionMin, unionMax, tmp);
    //            }

    //            for (const auto& sph : pmd->Spheres)
    //            {
    //                RBSnapshot tmp{};
    //                this->ExpandWithSphere(sph, object.Position, object.Forward,
    //                    object.Up, unionMin, unionMax, tmp);
    //            }

    //            break;

    //        default:
    //            break;
    //        }

    //        lop.RigidBodies.push_back(snap);
    //    }

    //    // Union sphere.
    //    if (maxRadius > 0.0f)
    //    {
    //        lop.HasPhysics = true;
    //        lop.BoundingSphereCenter[0] = dominantCenter[0];
    //        lop.BoundingSphereCenter[1] = dominantCenter[1];
    //        lop.BoundingSphereCenter[2] = dominantCenter[2];
    //        lop.BoundingSphereRadius = maxRadius;
    //    }

    //    // Union AABB.
    //    if (unionMin[0] != FLT_MAX)
    //    {
    //        lop.HasAABB = true;
    //        for (int k = 0; k < 3; ++k)
    //        {
    //            lop.AABBCenter[k] = (unionMin[k] + unionMax[k]) * 0.5f;
    //            lop.AABBHalfExtents[k] = (unionMax[k] - unionMin[k]) * 0.5f;
    //        }
    //    }

    //    snapshot.emplace(handle, lop);
    //}

    //g_pState->Domain->Phmo->SetPhmos(std::move(snapshot));
}

//void PhmoSystem::Cleanup()
//{
//    g_pState->Domain->Phmo->Cleanup();
//    g_pSystem->Debug->Log("[PhysicsSystem] INFO: Cleanup completed.");
//}


// Builds a right vector from Forward and Up (all assumed unit length).
void PhmoSystem::CrossProduct(const float a[3], const float b[3], float out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

void PhmoSystem::RotateByOrientation(const float modelPoint[3],
    const float forward[3], const float up[3], float outWorld[3])
{
    // Halo: X=forward, Y=left, Z=up, right-handed.
    // right = forward × up.
    float right[3];
    this->CrossProduct(forward, up, right);

    // modelPoint is on model-space: [0]=forward, [1]=left, [2]=up
    // outWorld = forward*x + left*y + up*z
    // But "left" = -right
    outWorld[0] = forward[0] * modelPoint[0] - 
        right[0] * modelPoint[1] + up[0] * modelPoint[2];

    outWorld[1] = forward[1] * modelPoint[0] - 
        right[1] * modelPoint[1] + up[1] * modelPoint[2];

    outWorld[2] = forward[2] * modelPoint[0] - 
        right[2] * modelPoint[1] + up[2] * modelPoint[2];
}

void PhmoSystem::TransformHalfExtents(const float modelHE[3],
    const float forward[3], const float up[3], float outWorldHE[3])
{
    float right[3];
    this->CrossProduct(forward, up, right);

    // Each world-axis component of the AABB half extent is the sum of
    // the absolute projections of all three local axes scaled by their
    // respective model half extent.
    for (int i = 0; i < 3; ++i)
    {
        outWorldHE[i] = fabsf(right[i]) * modelHE[0]
            + fabsf(forward[i]) * modelHE[1]
            + fabsf(up[i]) * modelHE[2];
    }
}

//void PhmoSystem::ExpandWithPill(const PhmoPill& pill, const float position[3],
//    const float forward[3], const float up[3], float unionMin[3],
//    float unionMax[3], RBSnapshot& snap)
//{
//    // A pill's AABB = union of two spheres at bottom and top
//    // World-space: rotate both endpoints, then expand by radius
//    float wBottom[3], wTop[3];
//    this->RotateByOrientation(pill.Bottom, forward, up, wBottom);
//    this->RotateByOrientation(pill.Top, forward, up, wTop);
//
//    for (int k = 0; k < 3; ++k)
//    {
//        wBottom[k] += position[k];
//        wTop[k] += position[k];
//    }
//
//    float r = pill.Radius;
//
//    // Compute AABB of the capsule
//    float lo[3]{};
//    float hi[3]{};
//    for (int k = 0; k < 3; ++k)
//    {
//        lo[k] = (std::min)(wBottom[k], wTop[k]) - r;
//        hi[k] = (std::max)(wBottom[k], wTop[k]) + r;
//    }
//
//    // Store in snap
//    for (int k = 0; k < 3; ++k)
//    {
//        snap.AABBCenter[k] = (lo[k] + hi[k]) * 0.5f;
//        snap.AABBHalfExtents[k] = (hi[k] - lo[k]) * 0.5f;
//    }
//
//    snap.HasAABB = true;
//
//    // Expand union
//    for (int k = 0; k < 3; ++k)
//    {
//        if (lo[k] < unionMin[k]) unionMin[k] = lo[k];
//        if (hi[k] > unionMax[k]) unionMax[k] = hi[k];
//    }
//}
//
//void PhmoSystem::ExpandWithSphere(const PhmoSphere& sphere, 
//    const float position[3], const float forward[3], const float up[3],
//    float unionMin[3], float unionMax[3], RBSnapshot& snap)
//{
//    float wCenter[3];
//    this->RotateByOrientation(sphere.Translation, forward, up, wCenter);
//
//    for (int k = 0; k < 3; ++k)
//    {
//        wCenter[k] += position[k];
//    }
//
//    float r = sphere.Radius;
//    for (int k = 0; k < 3; ++k)
//    {
//        snap.AABBCenter[k] = wCenter[k];
//        snap.AABBHalfExtents[k] = r;
//    }
//
//    snap.HasAABB = true;
//
//    for (int k = 0; k < 3; ++k)
//    {
//        if ((wCenter[k] - r) < unionMin[k]) unionMin[k] = wCenter[k] - r;
//        if ((wCenter[k] + r) > unionMax[k]) unionMax[k] = wCenter[k] + r;
//    }
//}
//
//void PhmoSystem::ExpandWithBox(const PhmoBox& box, const float position[3],
//    const float forward[3], const float up[3], float unionMin[3],
//    float unionMax[3], RBSnapshot& snap)
//{
//    // Box has its own local rotation (rotationI/J/K) and translation.
//    // We compose: world_axis = object_rotation * box_local_rotation
//    // Then transform half-extents conservatively.
//
//    // Object rotation matrix columns (from forward/up):
//    // right   = forward × up  (Halo: X=East, Y=North, Z=Up, right-handed)
//    float right[3];
//    this->CrossProduct(forward, up, right);
//    // Object axes: col0=forward, col1=-right (left), col2=up
//    // Actually we need to transform box local axes to world.
//    // box.rotationI/J/K are rows of the box's local rotation matrix.
//    // World axis = obj_rotation * box_local_axis
//
//    auto transformAxis = [&](const float local[3], float out[3]) {
//        // local is expressed in model space, rotate by object orientation
//        this->RotateByOrientation(local, forward, up, out);
//    };
//
//    float worldI[3], worldJ[3], worldK[3], worldT[3];
//    transformAxis(box.RotationI, worldI);
//    transformAxis(box.RotationJ, worldJ);
//    transformAxis(box.RotationK, worldK);
//    this->RotateByOrientation(box.Translation, forward, up, worldT);
//
//    for (int k = 0; k < 3; ++k)
//    {
//        worldT[k] += position[k];
//    }
//
//    // Conservative AABB from OBB:
//    // half-extent on world axis k = sum of |worldAxis[k]| * localHE.
//    float worldHE[3]{};
//    for (int k = 0; k < 3; ++k)
//    {
//        worldHE[k] = fabsf(worldI[k]) * box.HalfExtents[0] +
//            fabsf(worldJ[k]) * box.HalfExtents[1] +
//            fabsf(worldK[k]) * box.HalfExtents[2];
//    }
//
//    for (int k = 0; k < 3; ++k)
//    {
//        snap.AABBCenter[k] = worldT[k];
//        snap.AABBHalfExtents[k] = worldHE[k];
//    }
//
//    snap.HasAABB = true;
//
//    for (int k = 0; k < 3; ++k)
//    {
//        float lo = worldT[k] - worldHE[k];
//        float hi = worldT[k] + worldHE[k];
//        if (lo < unionMin[k]) unionMin[k] = lo;
//        if (hi > unionMax[k]) unionMax[k] = hi;
//    }
//}
//
//void PhmoSystem::ExpandWithTriangle(const PhmoTriangle& triangle,
//    const float position[3], const float forward[3],
//    const float up[3], float unionMin[3], float unionMax[3], RBSnapshot& snap)
//{
//    float wA[3];
//    float wB[3];
//    float wC[3];
//
//    this->RotateByOrientation(triangle.PointA, forward, up, wA);
//    this->RotateByOrientation(triangle.PointB, forward, up, wB);
//    this->RotateByOrientation(triangle.PointC, forward, up, wC);
//
//    float r = triangle.Radius;
//    float lo[3]{};
//    float hi[3]{};
//
//    for (int k = 0; k < 3; ++k)
//    {
//        wA[k] += position[k];
//        wB[k] += position[k];
//        wC[k] += position[k];
//        lo[k] = (std::min)({ wA[k], wB[k], wC[k] }) - r;
//        hi[k] = (std::max)({ wA[k], wB[k], wC[k] }) + r;
//
//        snap.AABBCenter[k] = (lo[k] + hi[k]) * 0.5f;
//        snap.AABBHalfExtents[k] = (hi[k] - lo[k]) * 0.5f;
//
//        if (lo[k] < unionMin[k]) unionMin[k] = lo[k];
//        if (hi[k] > unionMax[k]) unionMax[k] = hi[k];
//    }
//
//    snap.HasAABB = true;
//}
//
//void PhmoSystem::ExpandWithMultiSphere(const PhmoMultiSphere& multiSphere,
//    const float position[3], const float forward[3], const float up[3],
//    float unionMin[3], float unionMax[3], RBSnapshot& snap)
//{
//    float lo[3] = { FLT_MAX,  FLT_MAX,  FLT_MAX };
//    float hi[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
//
//    for (int32_t j = 0; j < multiSphere.Count; ++j)
//    {
//        float wC[3];
//        this->RotateByOrientation(multiSphere.Spheres[j].Center, forward, up, wC);
//
//        float r = multiSphere.Spheres[j].Radius;
//        for (int k = 0; k < 3; ++k)
//        {
//            wC[k] += position[k];
//            lo[k] = (std::min)(lo[k], wC[k] - r);
//            hi[k] = (std::max)(hi[k], wC[k] + r);
//        }
//    }
//
//    if (lo[0] != FLT_MAX)
//    {
//        for (int k = 0; k < 3; ++k)
//        {
//            snap.AABBCenter[k] = (lo[k] + hi[k]) * 0.5f;
//            snap.AABBHalfExtents[k] = (hi[k] - lo[k]) * 0.5f;
//            if (lo[k] < unionMin[k]) unionMin[k] = lo[k];
//            if (hi[k] > unionMax[k]) unionMax[k] = hi[k];
//        }
//
//        snap.HasAABB = true;
//    }
//}