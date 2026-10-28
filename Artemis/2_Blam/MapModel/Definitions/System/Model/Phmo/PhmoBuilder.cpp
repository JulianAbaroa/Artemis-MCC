module Resolved.Definitions.System;
import :Phmo;

import Resolved.Definitions.Type;

namespace
{
    using Resolved::Definitions::Type::Phmo::Shape;
    using Resolved::Definitions::Type::Phmo::Material;
    using Resolved::Definitions::Type::Phmo::PhantomType;
    using Resolved::Definitions::Type::Phmo::ShapeType;

    // Deepest nesting of lists, mopps and phantoms followed.
    constexpr int k_MaxShapeDepth{ 8 };

    // Reads the header that every primitive shape block shares.
    template <typename TEntry>
    auto MakeShape(const TEntry& entry, ShapeType type, std::int16_t index, bool isPhantom) -> Shape
    {
        return Shape
        {
            .Type = type,
            .Index = index,
            .Name = entry.Name,
            .MaterialIndex = entry.MaterialIndex,
            .MaterialFlags = entry.MaterialFlags,
            .PhantomTypeIndex = entry.PhantomTypeIndex,
            .CollisionGroup = entry.CollisionGroup,
            .IsPhantom = isPhantom,
        };
    }

    // return: The entry at index, or null if it is out of range.
    template <typename TEntry>
    auto EntryAt(const std::vector<TEntry>& block, std::int32_t index) -> const TEntry*
    {
        if (index < 0 || static_cast<std::size_t>(index) >= block.size()) return nullptr;

        return &block[static_cast<std::size_t>(index)];
    }
}

namespace Resolved::Definitions::System
{
    auto PhmoBuilder::Build(const PhmoObject& phmo) -> Phmo
    {
        Phmo out{};

        out.TagName = phmo.TagName;

        std::vector<std::size_t> listStarts{};
        listStarts.reserve(phmo.Lists.size());

        std::size_t next{ 0 };

        for (const auto& list : phmo.Lists)
        {
            listStarts.push_back(next);
            next += static_cast<std::size_t>((std::max)(list.ChildShapesSize, 0));
        }

        out.RigidBodies.reserve(phmo.RigidBodies.size());

        for (const auto& src : phmo.RigidBodies)
        {
            RigidBody& body{ out.RigidBodies.emplace_back() };

            body.NodeIndex = src.NodeIndex;
            body.RegionIndex = src.RegionIndex;
            body.PermutationIndex = src.PermutationIndex;
            body.Flags = src.Flags;
            body.MotionType = src.MotionType;
            body.ProxyCollisionGroup = src.ProxyCollisionGroup;
            body.Size = src.Size;
            body.CollisionGroupMask = src.CollisionGroupMask;
            body.Mass = src.Mass;
            body.RootShapeType = static_cast<ShapeType>(src.ShapeType);
            body.RootShapeIndex = src.ShapeIndex;

            this->AppendShapes(phmo, listStarts, body.RootShapeType, body.RootShapeIndex, false, 0, body);
        }

        out.Materials.reserve(phmo.Materials.size());

        for (const auto& src : phmo.Materials)
        {
            out.Materials.push_back(Material
            {
                .Name = src.Name,
                .GlobalMaterialName = src.GlobalMaterialName,
                .Flags = src.Flags,
                .PhantomTypeIndex = src.PhantomTypeIndex,
                .ProxyCollisionGroup = src.ProxyCollisionGroup,
                .CollisionGroup = src.CollisionGroup,
            });
        }

        out.PhantomTypes.reserve(phmo.PhantomTypes.size());

        for (const auto& src : phmo.PhantomTypes)
        {
            out.PhantomTypes.push_back(PhantomType
            {
                .Flags = src.Flags,
                .MinimumSize = src.MinimumSize,
                .MaximumSize = src.MaximumSize,
                .MarkerName = src.MarkerName,
                .AlignmentMarkerName = src.AlignmentMarkerName,
                .HookesLawE = src.HookesLawE,
                .LinearDeadRadius = src.LinearDeadRadius,
                .CenterAcceleration = src.CenterAcceleration,
                .CenterMaximumVelocity = src.CenterMaximumVelocity,
                .AxisAcceleration = src.AxisAcceleration,
                .AxisMaximumVelocity = src.AxisMaximumVelocity,
                .DirectionAcceleration = src.DirectionAcceleration,
                .DirectionMaximumVelocity = src.DirectionMaximumVelocity,
                .OrbitAcceleration = src.OrbitAcceleration,
                .OrbitMaximumVelocity = src.OrbitMaximumVelocity,
                .AlignmentHookesLawE = src.AlignmentHookesLawE,
                .AlignmentAcceleration = src.AlignmentAcceleration,
                .AlignmentMaximumVelocity = src.AlignmentMaximumVelocity,
            });
        }

        return out;
    }

    auto PhmoBuilder::AppendShapes(const PhmoObject& phmo, const std::vector<std::size_t>& listStarts, ShapeType type,
        std::int16_t index, bool isPhantom, int depth, RigidBody& out) -> void
    {
        if (depth > k_MaxShapeDepth) return;

        const auto appendPrimitive = [&](const auto& block)
        {
            const auto* entry{ EntryAt(block, index) };
            if (entry) out.Shapes.push_back(MakeShape(*entry, type, index, isPhantom));
        };

        switch (type)
        {
        case ShapeType::Sphere: appendPrimitive(phmo.Spheres); break;
        case ShapeType::Pill: appendPrimitive(phmo.Pills); break;
        case ShapeType::Box: appendPrimitive(phmo.Boxes); break;
        case ShapeType::Triangle: appendPrimitive(phmo.Triangles); break;
        case ShapeType::Polyhedron: appendPrimitive(phmo.Polyhedra); break;
        case ShapeType::MultiSphere: appendPrimitive(phmo.MultiSpheres); break;

        case ShapeType::Phantom:
        {
            const auto* phantom{ EntryAt(phmo.Phantoms, index) };
            if (!phantom) return;

            this->AppendShapes(phmo, listStarts, static_cast<ShapeType>(phantom->ShapeType), phantom->ShapeIndex, true, depth + 1, out);
            break;
        }

        case ShapeType::Mopp:
        {
            const auto* mopp{ EntryAt(phmo.Mopps, index) };
            if (!mopp) return;

            this->AppendShapes(phmo, listStarts, static_cast<ShapeType>(mopp->ShapeType), mopp->ShapeIndex, isPhantom, depth + 1, out);
            break;
        }

        case ShapeType::List:
        {
            const auto* list{ EntryAt(phmo.Lists, index) };
            if (!list) return;

            const std::size_t start{ listStarts[static_cast<std::size_t>(index)] };
            const std::size_t end{ (std::min)(start + static_cast<std::size_t>((std::max)(list->ChildShapesSize, 0)), phmo.ListShapes.size()) };

            for (std::size_t child{ start }; child < end; ++child)
            {
                const auto& entry{ phmo.ListShapes[child] };
                this->AppendShapes(phmo, listStarts, static_cast<ShapeType>(entry.ShapeType), entry.ShapeIndex, isPhantom, depth + 1, out);
            }
            break;
        }

        default: break;
        }
    }
}