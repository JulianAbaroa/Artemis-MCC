export module Common.Reflect.Type:Math;

import :Core;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec2;
    using Common::Math::Type::Vec3;
    using Common::Math::Type::Vec4;
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Node;
}

export namespace Common::Reflect::Type
{
    // Reflects the fields of Vec2.
    template <>
    struct Fields<Vec2>
    {
        static constexpr auto k_Value{ std::tuple{
            MakeField("X", &Vec2::X),
            MakeField("Y", &Vec2::Y),
        } };
    };

    // Reflects the fields of Vec3.
    template <>
    struct Fields<Vec3>
    {
        static constexpr auto k_Value{ std::tuple{
            MakeField("X", &Vec3::X),
            MakeField("Y", &Vec3::Y),
            MakeField("Z", &Vec3::Z),
        } };
    };

    // Reflects the fields of Vec4.
    template <>
    struct Fields<Vec4>
    {
        static constexpr auto k_Value{ std::tuple{
            MakeField("X", &Vec4::X),
            MakeField("Y", &Vec4::Y),
            MakeField("Z", &Vec4::Z),
            MakeField("W", &Vec4::W),
        } };
    };

    // Reflects the fields of Triangle.
    template <>
    struct Fields<Triangle>
    {
        static constexpr auto k_Value{ std::tuple{
            MakeField("A", &Triangle::A),
            MakeField("B", &Triangle::B),
            MakeField("C", &Triangle::C),
            MakeField("SurfaceFlags", &Triangle::SurfaceFlags),
            MakeField("Material", &Triangle::Material),
        } };
    };

    // Reflects the fields of Node.
    template <>
    struct Fields<Node>
    {
        static constexpr auto k_Value{ std::tuple{
            MakeField("Name", &Node::Name),
            MakeField("ParentIndex", &Node::ParentIndex),
            MakeField("NextSiblingIndex", &Node::NextSiblingIndex),
            MakeField("FirstChildIndex", &Node::FirstChildIndex),
            MakeField("DefaultTranslation", &Node::DefaultTranslation),
            MakeField("DefaultRotation", &Node::DefaultRotation),
            MakeField("InverseScale", &Node::InverseScale),
            MakeField("DoesNotAnimate", &Node::DoesNotAnimate),
        } };
    };
}