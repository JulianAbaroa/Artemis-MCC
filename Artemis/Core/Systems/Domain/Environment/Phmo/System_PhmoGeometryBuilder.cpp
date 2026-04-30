#include "pch.h"
#include "Core/Systems/Domain/Environment/Phmo/System_PhmoGeometryBuilder.h"
#include "Core/Types/Domain/Domains/Environment/PhmoGeometry.h"
#include "Generated/Phmo/PhmoObject.h"
#include <algorithm>
#include <limits>

PhmoGeometry System_PhmoGeometryBuilder::BuildGeometry(const PhmoObject& phmo)
{
    PhmoGeometry out;
    out.TagName = phmo.TagName;

    this->BuildRigidBodies(phmo, out);
    this->BuildNodes(phmo, out);
    this->BuildBounds(out);

    return out;
}

void System_PhmoGeometryBuilder::BuildRigidBodies(const PhmoObject& phmo, PhmoGeometry& out)
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

std::vector<PhmoShape> System_PhmoGeometryBuilder::ResolveShapes(
    const PhmoObject& phmo,
    uint16_t shapeType,
    int16_t shapeIndex)
{
    std::vector<PhmoShape> result;

    if (shapeIndex < 0) return result;

    if (shapeType == k_ShapeList)
    {
        if (shapeIndex >= static_cast<int16_t>(phmo.Lists.size()))
            return result;

        const auto& list = phmo.Lists[shapeIndex];

        // ListShapes are global entries, the List references a consecutive range
        // within phmo.ListShapes by ChildShapesSize from its position. 
        // We iterate through all ListShapes, searching for those that
        // belong to this List by shape index.

        // In Havok, the List points to the first child via RuntimeListPointer,
        // but in the tag, the ListShapes are ordered, and ChildShapesSize
        // indicates how many there are starting from the offset UserData / entry size.

        // We use the index of List * ChildShapesSize as a base.

        // Safer alternative: iterate through all ListShapes and take
        // the first ChildShapesSize starting from the calculated offset.

        const int32_t count = list.ChildShapesSize;
        const int32_t total = static_cast<int32_t>(phmo.ListShapes.size());

        // The base offset in ListShapes for this List is:
        // shapeIndex * count (each List occupies a consecutive block)
        const int32_t base = shapeIndex * count;

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
        // MOPP is a BVH over other shapes, for Level B we take
        // the AABB of MOPP (CodeInfoCopy + Scale) as an approximate polyhedron.
        if (shapeIndex < static_cast<int16_t>(phmo.Mopps.size()))
        {
            const auto& mopp = phmo.Mopps[shapeIndex];
            PhmoShape shape;
            shape.Type = PhmoShapeType::Polyhedron;
            // CodeInfoCopy is the MOPP offset/scale, we use it as the center.
            shape.Polyhedron.AABBCenter = this->MakeVec3(mopp.CodeInfoCopy);
            // Without accurate half-extents at Level B, we leave it at 0 for upgrade.
            shape.Polyhedron.AABBHalfExtents = {};
            result.push_back(shape);
        }
    }
    else
    {
        PhmoShape primitive;
        if (this->ResolvePrimitive(phmo, shapeType, shapeIndex, primitive))
            result.push_back(primitive);
    }

    return result;
}

bool System_PhmoGeometryBuilder::ResolvePrimitive(
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
        out = this->MakePolyhedron(phmo.Polyhedra[shapeIndex]);
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

PhmoShape System_PhmoGeometryBuilder::MakeSphere(const Phmo_SpheresEntry& src)
{
    PhmoShape s;
    s.Type = PhmoShapeType::Sphere;
    s.Sphere.Center = MakeVec3(src.Translation);
    s.Sphere.Radius = src.Radius;
    return s;
}

PhmoShape System_PhmoGeometryBuilder::MakePill(const Phmo_PillsEntry& src)
{
    PhmoShape s;
    s.Type = PhmoShapeType::Pill;
    s.Pill.Bottom = this->MakeVec3(src.Bottom);
    s.Pill.Top = this->MakeVec3(src.Top);
    s.Pill.Radius = src.Radius;
    return s;
}

PhmoShape System_PhmoGeometryBuilder::MakeBox(const Phmo_BoxesEntry& src)
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

PhmoShape System_PhmoGeometryBuilder::MakePolyhedron(const Phmo_PolyhedraEntry& src)
{
    PhmoShape s;
    s.Type = PhmoShapeType::Polyhedron;
    s.Polyhedron.AABBCenter = this->MakeVec3(src.AxisAlignedBoundingBoxCenter);
    s.Polyhedron.AABBHalfExtents = this->MakeVec3(src.AxisAlignedBoundingBoxHalfExtents);
    return s;
}

PhmoShape System_PhmoGeometryBuilder::MakeMultiSphere(const Phmo_MultiSpheresEntry& src)
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

PhmoVec3 System_PhmoGeometryBuilder::MakeVec3(const Vec3& v) { return { v.X, v.Y, v.Z }; }

void System_PhmoGeometryBuilder::BuildNodes(const PhmoObject& phmo, PhmoGeometry& out)
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

void System_PhmoGeometryBuilder::BuildBounds(PhmoGeometry& out)
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