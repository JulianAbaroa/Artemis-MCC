export module Resolved.Definitions.Reflect:Phmo;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Phmo::Shape;
    using Resolved::Definitions::Type::Phmo::Material;
    using Resolved::Definitions::Type::Phmo::RigidBody;
    using Resolved::Definitions::Type::Phmo::PhantomType;
    using Resolved::Definitions::Type::Phmo::Phmo;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Shape>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Type", &Shape::Type),
            MakeField("Index", &Shape::Index),
            MakeField("Name", &Shape::Name),
            MakeField("MaterialIndex", &Shape::MaterialIndex),
            MakeField("MaterialFlags", &Shape::MaterialFlags),
            MakeField("PhantomTypeIndex", &Shape::PhantomTypeIndex),
            MakeField("CollisionGroup", &Shape::CollisionGroup),
            MakeField("IsPhantom", &Shape::IsPhantom),
        };
    };

    template <>
    struct Fields<Material>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Name", &Material::Name),
            MakeField("GlobalMaterialName", &Material::GlobalMaterialName),
            MakeField("Flags", &Material::Flags),
            MakeField("PhantomTypeIndex", &Material::PhantomTypeIndex),
            MakeField("ProxyCollisionGroup", &Material::ProxyCollisionGroup),
            MakeField("CollisionGroup", &Material::CollisionGroup),
        };
    };

    template <>
    struct Fields<RigidBody>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("NodeIndex", &RigidBody::NodeIndex),
            MakeField("RegionIndex", &RigidBody::RegionIndex),
            MakeField("PermutationIndex", &RigidBody::PermutationIndex),
            MakeField("Flags", &RigidBody::Flags),
            MakeField("MotionType", &RigidBody::MotionType),
            MakeField("ProxyCollisionGroup", &RigidBody::ProxyCollisionGroup),
            MakeField("Size", &RigidBody::Size),
            MakeField("CollisionGroupMask", &RigidBody::CollisionGroupMask),
            MakeField("Mass", &RigidBody::Mass),
            MakeField("RootShapeType", &RigidBody::RootShapeType),
            MakeField("RootShapeIndex", &RigidBody::RootShapeIndex),
            MakeField("Shapes", &RigidBody::Shapes),
        };
    };

    template <>
    struct Fields<PhantomType>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Flags", &PhantomType::Flags),
            MakeField("MinimumSize", &PhantomType::MinimumSize),
            MakeField("MaximumSize", &PhantomType::MaximumSize),
            MakeField("MarkerName", &PhantomType::MarkerName),
            MakeField("AlignmentMarkerName", &PhantomType::AlignmentMarkerName),
            MakeField("HookesLawE", &PhantomType::HookesLawE),
            MakeField("LinearDeadRadius", &PhantomType::LinearDeadRadius),
            MakeField("CenterAcceleration", &PhantomType::CenterAcceleration),
            MakeField("CenterMaximumVelocity", &PhantomType::CenterMaximumVelocity),
            MakeField("AxisAcceleration", &PhantomType::AxisAcceleration),
            MakeField("AxisMaximumVelocity", &PhantomType::AxisMaximumVelocity),
            MakeField("DirectionAcceleration", &PhantomType::DirectionAcceleration),
            MakeField("DirectionMaximumVelocity", &PhantomType::DirectionMaximumVelocity),
            MakeField("OrbitAcceleration", &PhantomType::OrbitAcceleration),
            MakeField("OrbitMaximumVelocity", &PhantomType::OrbitMaximumVelocity),
            MakeField("AlignmentHookesLawE", &PhantomType::AlignmentHookesLawE),
            MakeField("AlignmentAcceleration", &PhantomType::AlignmentAcceleration),
            MakeField("AlignmentMaximumVelocity", &PhantomType::AlignmentMaximumVelocity),
        };
    };

    template <>
    struct Fields<Phmo>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("TagName", &Phmo::TagName),
            MakeField("RigidBodies", &Phmo::RigidBodies),
            MakeField("Materials", &Phmo::Materials),
            MakeField("PhantomTypes", &Phmo::PhantomTypes),
        };
    };
}