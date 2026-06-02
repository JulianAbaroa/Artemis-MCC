#include "pch.h"

#include "System_PhmoBuilder.h"

#include "Core/Types/Environment/PhmoGeometry.h"

#include "Generated/Phmo/PhmoObject.h"

#include <algorithm>
#include <limits>

PhmoGeometry System_PhmoBuilder::Build(const PhmoObject& phmo)
{
    PhmoGeometry out;
    out.TagName = phmo.TagName;

    this->BuildRigidBodies(phmo, out);
    this->BuildNodes(phmo, out);
    this->BuildBounds(out);

    return out;
}

void System_PhmoBuilder::BuildRigidBodies(const PhmoObject& phmo, PhmoGeometry& out)
{
    out.RigidBodies.reserve(phmo.RigidBodies.size());

    for (const auto& src : phmo.RigidBodies)
    {
        PhmoRigidBody rb;
        rb.NodeIndex = src.NodeIndex;
        rb.RegionIndex = src.RegionIndex;
        rb.PermutationIndex = src.PermutationIndex;
        rb.BoundingSphereOffset = this->MakeVec3(src.BoundingSphereOffset);
        rb.BoundingSphereRadius = src.BoundingSphereRadius;
        rb.CollisionGroupMask = src.CollisionGroupMask;

        rb.Shapes = this->ResolveShapes(phmo, src.ShapeType, src.ShapeIndex);

        out.RigidBodies.push_back(std::move(rb));
    }
}

std::vector<PhmoShape> System_PhmoBuilder::ResolveShapes(
    const PhmoObject& phmo,
    uint16_t shapeType,
    int16_t shapeIndex)
{
    std::vector<PhmoShape> result;

    if (shapeIndex < 0) return result;

    if (shapeType == k_ShapeList)
    {
        if (shapeIndex >= static_cast<int16_t>(phmo.Lists.size())) return result;

        const auto& list = phmo.Lists[shapeIndex];
        const int32_t count = list.ChildShapesSize;
        const int32_t total = static_cast<int32_t>(phmo.ListShapes.size());

        // UserData es byte offset desde inicio del bloque ListShapes.
        const int32_t base =
            static_cast<int32_t>(list.UserData) /
            static_cast<int32_t>(sizeof(Phmo_ListShapesEntry));

        for (int32_t i = 0; i < count; ++i)
        {
            const int32_t idx = base + i;
            if (idx >= total) break;

            const auto& ls = phmo.ListShapes[idx];
            PhmoShape primitive;
            if (this->ResolvePrimitive(phmo, ls.ShapeType, ls.ShapeIndex, primitive))
                result.push_back(primitive);
        }
    }
    else if (shapeType == k_ShapeMOPP)
    {
        if (shapeIndex >= static_cast<int16_t>(phmo.Mopps.size()))
            return result;

        const auto& mopp = phmo.Mopps[shapeIndex];

        // El MOPP apunta a su child shape, seguirla.
        std::vector<PhmoShape> inner = this->ResolveShapes(
            phmo, mopp.ShapeType, mopp.ShapeIndex);

        result.insert(result.end(), inner.begin(), inner.end());
    }
    else if (shapeType == k_ShapePhantom)
    {
        if (shapeIndex >= static_cast<int16_t>(phmo.Phantoms.size()))
            return result;

        const auto& phantom = phmo.Phantoms[shapeIndex];

        std::vector<PhmoShape> inner = this->ResolveShapes(
            phmo, phantom.ShapeType, phantom.ShapeIndex);

        result.insert(result.end(), inner.begin(), inner.end());
    }
    else
    {
        PhmoShape primitive;
        if (this->ResolvePrimitive(phmo, shapeType, shapeIndex, primitive))
            result.push_back(primitive);
    }

    return result;
}

bool System_PhmoBuilder::ResolvePrimitive(
    const PhmoObject& phmo,
    uint16_t shapeType,
    int16_t shapeIndex,
    PhmoShape& out)
{
    if (shapeIndex < 0) return false;

    switch (shapeType)
    {
    case k_ShapeSphere:
        if (shapeIndex >= static_cast<int16_t>(phmo.Spheres.size())) return false;
        out = this->MakeSphere(phmo.Spheres[shapeIndex]);
        return true;

    case k_ShapePill:
        if (shapeIndex >= static_cast<int16_t>(phmo.Pills.size())) return false;
        out = this->MakePill(phmo.Pills[shapeIndex]);
        return true;

    case k_ShapeBox:
        if (shapeIndex >= static_cast<int16_t>(phmo.Boxes.size())) return false;
        out = this->MakeBox(phmo.Boxes[shapeIndex]);
        return true;

    case k_ShapePolyhedron:
        if (shapeIndex >= static_cast<int16_t>(phmo.Polyhedra.size())) return false;
        out = this->MakePolyhedron(phmo, phmo.Polyhedra[shapeIndex], shapeIndex);
        return true;

    case k_ShapeMultiSphere:
        if (shapeIndex >= static_cast<int16_t>(phmo.MultiSpheres.size())) return false;
        out = this->MakeMultiSphere(phmo.MultiSpheres[shapeIndex]);
        return true;

    case k_ShapeTriangle:
        return false;

    default:
        return false;
    }
}

PhmoShape System_PhmoBuilder::MakeSphere(const Phmo_SpheresEntry& src)
{
    PhmoShape s;
    s.Type = PhmoShapeType::Sphere;
    s.Sphere.Center = MakeVec3(src.Translation);
    s.Sphere.Radius = src.Radius;
    return s;
}

PhmoShape System_PhmoBuilder::MakePill(const Phmo_PillsEntry& src)
{
    PhmoShape s;
    s.Type = PhmoShapeType::Pill;
    s.Pill.Bottom = this->MakeVec3(src.Bottom);
    s.Pill.Top = this->MakeVec3(src.Top);
    s.Pill.Radius = src.Radius;
    return s;
}

PhmoShape System_PhmoBuilder::MakeBox(const Phmo_BoxesEntry& src)
{
    PhmoShape s;
    s.Type = PhmoShapeType::Box;
    s.Box.Center = this->MakeVec3(src.Translation);
    s.Box.HalfExtents = this->MakeVec3(src.HalfExtents);
    s.Box.RotationI = this->MakeVec3(src.RotationI);
    s.Box.RotationJ = this->MakeVec3(src.RotationJ);
    s.Box.RotationK = this->MakeVec3(src.RotationK);
    return s;
}

PhmoShape System_PhmoBuilder::MakePolyhedron(
    const PhmoObject& phmo,
    const Phmo_PolyhedraEntry& src,
    int16_t polyIndex)
{
    PhmoShape s;
    s.Type = PhmoShapeType::Polyhedron;
    s.Polyhedron.AABBCenter = this->MakeVec3(src.AxisAlignedBoundingBoxCenter);
    s.Polyhedron.AABBHalfExtents = this->MakeVec3(src.AxisAlignedBoundingBoxHalfExtents);

    const int32_t fvCount = src.FourVectorsSize;
    const int32_t numVerts = src.NumberOfVertices;
    const int32_t totalFV = static_cast<int32_t>(phmo.PolyhedronFourVectors.size());

    if (fvCount <= 0 || numVerts <= 0 || totalFV == 0)
        return s;

    int32_t fvBase = 0;
    for (int16_t i = 0; i < polyIndex; ++i)
        fvBase += phmo.Polyhedra[i].FourVectorsSize;

    if (fvBase < 0 || fvBase >= totalFV)
        return s;

    s.Polyhedron.Vertices.reserve(numVerts);

    int32_t vertsSeen = 0;
    for (int32_t i = 0; i < fvCount && (fvBase + i) < totalFV; ++i)
    {
        const auto& fv = phmo.PolyhedronFourVectors[fvBase + i];

        const float xs[4] = {
            fv.FourVectorsX.X, fv.FourVectorsX.Y,
            fv.FourVectorsX.Z, fv.WFourVectorsX };
        const float ys[4] = {
            fv.FourVectorsY.X, fv.FourVectorsY.Y,
            fv.FourVectorsY.Z, fv.WFourVectorsY };
        const float zs[4] = {
            fv.FourVectorsZ.X, fv.FourVectorsZ.Y,
            fv.FourVectorsZ.Z, fv.WFourVectorsZ };

        for (int32_t j = 0; j < 4 && vertsSeen < numVerts; ++j, ++vertsSeen)
            s.Polyhedron.Vertices.push_back({ xs[j], ys[j], zs[j] });
    }

    return s;
}

PhmoShape System_PhmoBuilder::MakeMultiSphere(const Phmo_MultiSpheresEntry& src)
{
    PhmoShape s;
    s.Type = PhmoShapeType::MultiSphere;

    const int32_t count = (std::min)(src.NumberOfSpheres, 8);
    s.MultiSphere.Spheres.reserve(count);

    // The 8 slots are hardcoded in the entry as Sphere0..7
    // W = radius in Havok's vector4 format.
    const struct { const Vec3& pos; float w; } slots[8] =
    {
        { src.Sphere0, src.WSphere0 },
        { src.Sphere1, src.WSphere1 },
        { src.Sphere2, src.WSphere2 },
        { src.Sphere3, src.WSphere3 },
        { src.Sphere4, src.WSphere4 },
        { src.Sphere5, src.WSphere5 },
        { src.Sphere6, src.WSphere6 },
        { src.Sphere7, src.WSphere7 },
    };

    for (int32_t i = 0; i < count; ++i)
    {
        PhmoMultiSphereShape::Sphere sphere;
        sphere.Center = this->MakeVec3(slots[i].pos);
        sphere.Radius = slots[i].w;
        s.MultiSphere.Spheres.push_back(sphere);
    }

    return s;
}

PhmoVec3 System_PhmoBuilder::MakeVec3(const Vec3& v) { return { v.X, v.Y, v.Z }; }

void System_PhmoBuilder::BuildNodes(const PhmoObject& phmo, PhmoGeometry& out)
{
    out.Nodes.reserve(phmo.Nodes.size());

    for (const auto& src : phmo.Nodes)
    {
        PhmoNode node;
        node.Name = std::to_string(src.Name);
        node.ParentIndex = src.ParentIndex;
        node.SiblingIndex = src.SiblingIndex;
        node.ChildIndex = src.ChildIndex;
        node.DoesNotAnimate = (src.Flags & (1 << 0)) != 0;
        out.Nodes.push_back(node);
    }
}

void System_PhmoBuilder::BuildBounds(PhmoGeometry& out)
{
    if (out.RigidBodies.empty())
    {
        out.BoundsMin = {};
        out.BoundsMax = {};
        return;
    }

    constexpr float kMax = (std::numeric_limits<float>::max)();
    out.BoundsMin = { kMax,  kMax,  kMax };
    out.BoundsMax = { -kMax, -kMax, -kMax };

    for (const auto& rb : out.RigidBodies)
    {
        const float r = rb.BoundingSphereRadius;
        const auto& c = rb.BoundingSphereOffset;

        out.BoundsMin.X = (std::min)(out.BoundsMin.X, c.X - r);
        out.BoundsMin.Y = (std::min)(out.BoundsMin.Y, c.Y - r);
        out.BoundsMin.Z = (std::min)(out.BoundsMin.Z, c.Z - r);

        out.BoundsMax.X = (std::max)(out.BoundsMax.X, c.X + r);
        out.BoundsMax.Y = (std::max)(out.BoundsMax.Y, c.Y + r);
        out.BoundsMax.Z = (std::max)(out.BoundsMax.Z, c.Z + r);
    }
}