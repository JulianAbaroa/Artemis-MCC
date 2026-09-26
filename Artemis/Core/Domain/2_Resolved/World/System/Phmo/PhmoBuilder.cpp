module Resolved.World.System;
import :Phmo;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using Phmo_ListShapesEntry = Map::Tag::Type::Phmo::Structure::Phmo_ListShapesEntry;
    using RigidBody = Resolved::World::Type::Phmo::RigidBody;

    using Resolved::World::Type::Constant::k_ShapeSphere;
    using Resolved::World::Type::Constant::k_ShapePill;
    using Resolved::World::Type::Constant::k_ShapeBox;
    using Resolved::World::Type::Constant::k_ShapeTriangle;
    using Resolved::World::Type::Constant::k_ShapePolyhedron;
    using Resolved::World::Type::Constant::k_ShapeMultiSphere;
    using Resolved::World::Type::Constant::k_ShapePhantom;
    using Resolved::World::Type::Constant::k_ShapeList;
    using Resolved::World::Type::Constant::k_ShapeMOPP;
}

namespace Resolved::World::System
{
    auto PhmoBuilder::Build(const PhmoObject& phmo) -> ResolvedPhmo
    {
        ResolvedPhmo out;
        out.TagName = phmo.TagName;

        this->BuildRigidBodies(phmo, out);
        this->BuildNodes(phmo, out);
        this->BuildBounds(out);

        return out;
    }

    auto PhmoBuilder::BuildRigidBodies(const PhmoObject& phmo, ResolvedPhmo& out) -> void
    {
        out.RigidBodies.reserve(phmo.RigidBodies.size());

        for (const auto& src : phmo.RigidBodies)
        {
            RigidBody rb;
            rb.NodeIndex = src.NodeIndex;
            rb.RegionIndex = src.RegionIndex;
            rb.PermutationIndex = src.PermutationIndex;
            rb.BoundingSphereOffset = std::bit_cast<Common::Math::Type::Vec3>(src.BoundingSphereOffset);
            rb.BoundingSphereRadius = src.BoundingSphereRadius;
            rb.CollisionGroupMask = src.CollisionGroupMask;

            rb.Shapes = this->ResolveShapes(phmo, src.ShapeType, src.ShapeIndex);

            out.RigidBodies.push_back(std::move(rb));
        }
    }

    auto PhmoBuilder::BuildNodes(const PhmoObject& phmo, ResolvedPhmo& out) -> void
    {
        out.Nodes.reserve(phmo.Nodes.size());

        for (const auto& src : phmo.Nodes)
        {
            Node node;
            node.Name = std::to_string(src.Name);
            node.ParentIndex = src.ParentIndex;
            node.NextSiblingIndex = src.SiblingIndex;
            node.FirstChildIndex = src.ChildIndex;
            node.DoesNotAnimate = (src.Flags & (1 << 0)) != 0;
            out.Nodes.push_back(node);
        }
    }

    auto PhmoBuilder::BuildBounds(ResolvedPhmo& out) -> void
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

    auto PhmoBuilder::ResolveShapes(const PhmoObject& phmo,
        std::uint16_t shapeType, std::int16_t shapeIndex) -> std::vector<Shape>
    {
        std::vector<Shape> result;

        if (shapeIndex < 0) return result;

        if (shapeType == k_ShapeList)
        {
            if (shapeIndex >= static_cast<std::int16_t>(phmo.Lists.size())) return result;

            const auto& list = phmo.Lists[shapeIndex];
            const std::int32_t count = list.ChildShapesSize;
            const std::int32_t total = static_cast<std::int32_t>(phmo.ListShapes.size());

            const std::int32_t base =
                static_cast<std::int32_t>(list.UserData) /
                static_cast<std::int32_t>(sizeof(Phmo_ListShapesEntry));

            for (std::int32_t i = 0; i < count; ++i)
            {
                const std::int32_t idx = base + i;
                if (idx >= total) break;

                const auto& ls = phmo.ListShapes[idx];
                Shape primitive;
                if (this->ResolvePrimitive(phmo, ls.ShapeType, ls.ShapeIndex, primitive))
                    result.push_back(primitive);
            }
        }
        else if (shapeType == k_ShapeMOPP)
        {
            if (shapeIndex >= static_cast<std::int16_t>(phmo.Mopps.size()))
                return result;

            const auto& mopp = phmo.Mopps[shapeIndex];

            std::vector<Shape> inner = this->ResolveShapes(
                phmo, mopp.ShapeType, mopp.ShapeIndex);

            result.insert(result.end(), inner.begin(), inner.end());
        }
        else if (shapeType == k_ShapePhantom)
        {
            if (shapeIndex >= static_cast<std::int16_t>(phmo.Phantoms.size()))
                return result;

            const auto& phantom = phmo.Phantoms[shapeIndex];

            std::vector<Shape> inner = this->ResolveShapes(
                phmo, phantom.ShapeType, phantom.ShapeIndex);

            result.insert(result.end(), inner.begin(), inner.end());
        }
        else
        {
            Shape primitive;
            if (this->ResolvePrimitive(phmo, shapeType, shapeIndex, primitive))
                result.push_back(primitive);
        }

        return result;
    }

    auto PhmoBuilder::ResolvePrimitive(const PhmoObject& phmo,
        std::uint16_t shapeType, std::int16_t shapeIndex, Shape& out) -> bool
    {
        if (shapeIndex < 0) return false;

        switch (shapeType)
        {
        case k_ShapeSphere:
            if (shapeIndex >= static_cast<std::int16_t>(phmo.Spheres.size())) return false;
            out = this->MakeSphere(phmo.Spheres[shapeIndex]);
            return true;

        case k_ShapePill:
            if (shapeIndex >= static_cast<std::int16_t>(phmo.Pills.size())) return false;
            out = this->MakePill(phmo.Pills[shapeIndex]);
            return true;

        case k_ShapeBox:
            if (shapeIndex >= static_cast<std::int16_t>(phmo.Boxes.size())) return false;
            out = this->MakeBox(phmo.Boxes[shapeIndex]);
            return true;

        case k_ShapePolyhedron:
            if (shapeIndex >= static_cast<std::int16_t>(phmo.Polyhedra.size())) return false;
            out = this->MakePolyhedron(phmo, phmo.Polyhedra[shapeIndex], shapeIndex);
            return true;

        case k_ShapeMultiSphere:
            if (shapeIndex >= static_cast<std::int16_t>(phmo.MultiSpheres.size())) return false;
            out = this->MakeMultiSphere(phmo.MultiSpheres[shapeIndex]);
            return true;

        case k_ShapeTriangle:
            return false;

        default:
            return false;
        }
    }

    auto PhmoBuilder::MakeSphere(const Phmo_SpheresEntry& src) -> Shape
    {
        Shape s;
        s.Type = ShapeType::Sphere;
        s.Sphere.Center = std::bit_cast<Common::Math::Type::Vec3>(src.Translation);
        s.Sphere.Radius = src.Radius;
        return s;
    }

    auto PhmoBuilder::MakePill(const Phmo_PillsEntry& src) -> Shape
    {
        Shape s;
        s.Type = ShapeType::Pill;
        s.Pill.Bottom = std::bit_cast<Common::Math::Type::Vec3>(src.Bottom);
        s.Pill.Top = std::bit_cast<Common::Math::Type::Vec3>(src.Top);
        s.Pill.Radius = src.Radius;
        return s;
    }

    auto PhmoBuilder::MakeBox(const Phmo_BoxesEntry& src) -> Shape
    {
        Shape s;
        s.Type = ShapeType::Box;
        s.Box.Center = std::bit_cast<Common::Math::Type::Vec3>(src.Translation);
        s.Box.HalfExtents = std::bit_cast<Common::Math::Type::Vec3>(src.HalfExtents);
        s.Box.RotationI = std::bit_cast<Common::Math::Type::Vec3>(src.RotationI);
        s.Box.RotationJ = std::bit_cast<Common::Math::Type::Vec3>(src.RotationJ);
        s.Box.RotationK = std::bit_cast<Common::Math::Type::Vec3>(src.RotationK);
        return s;
    }

    auto PhmoBuilder::MakePolyhedron(const PhmoObject& phmo,
        const Phmo_PolyhedraEntry& src, std::int16_t polyIndex) -> Shape
    {
        Shape s;
        s.Type = ShapeType::Polyhedron;
        s.Polyhedron.AABBCenter = std::bit_cast<Common::Math::Type::Vec3>(src.AxisAlignedBoundingBoxCenter);
        s.Polyhedron.AABBHalfExtents = std::bit_cast<Common::Math::Type::Vec3>(src.AxisAlignedBoundingBoxHalfExtents);

        const int32_t fvCount = src.FourVectorsSize;
        const int32_t numVerts = src.NumberOfVertices;
        const int32_t totalFV = static_cast<int32_t>(phmo.PolyhedronFourVectors.size());

        if (fvCount <= 0 || numVerts <= 0 || totalFV == 0)
            return s;

        std::int32_t fvBase = 0;
        for (std::int16_t i = 0; i < polyIndex; ++i)
            fvBase += phmo.Polyhedra[i].FourVectorsSize;

        if (fvBase < 0 || fvBase >= totalFV)
            return s;

        s.Polyhedron.Vertices.reserve(numVerts);

        std::int32_t vertsSeen = 0;
        for (std::int32_t i = 0; i < fvCount && (fvBase + i) < totalFV; ++i)
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

            for (std::int32_t j = 0; j < 4 && vertsSeen < numVerts; ++j, ++vertsSeen)
                s.Polyhedron.Vertices.push_back({ xs[j], ys[j], zs[j] });
        }

        return s;
    }

    auto PhmoBuilder::MakeMultiSphere(const Phmo_MultiSpheresEntry& src) -> Shape
    {
        Shape s;
        s.Type = ShapeType::MultiSphere;

        const std::int32_t count = (std::min)(src.NumberOfSpheres, 8);
        s.MultiSphere.Spheres.reserve(count);

        const struct { const Vec3& pos; float w; } slots[8] =
        {
            { std::bit_cast<Common::Math::Type::Vec3>(src.Sphere0), src.WSphere0 },
            { std::bit_cast<Common::Math::Type::Vec3>(src.Sphere1), src.WSphere1 },
            { std::bit_cast<Common::Math::Type::Vec3>(src.Sphere2), src.WSphere2 },
            { std::bit_cast<Common::Math::Type::Vec3>(src.Sphere3), src.WSphere3 },
            { std::bit_cast<Common::Math::Type::Vec3>(src.Sphere4), src.WSphere4 },
            { std::bit_cast<Common::Math::Type::Vec3>(src.Sphere5), src.WSphere5 },
            { std::bit_cast<Common::Math::Type::Vec3>(src.Sphere6), src.WSphere6 },
            { std::bit_cast<Common::Math::Type::Vec3>(src.Sphere7), src.WSphere7 },
        };

        for (std::int32_t i = 0; i < count; ++i)
        {
            MultiSphere::Sphere sphere;
            sphere.Center = std::bit_cast<Common::Math::Type::Vec3>(slots[i].pos);
            sphere.Radius = slots[i].w;
            s.MultiSphere.Spheres.push_back(sphere);
        }

        return s;
    }
}