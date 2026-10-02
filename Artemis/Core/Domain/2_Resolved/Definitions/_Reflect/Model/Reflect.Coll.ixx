export module Resolved.Definitions.Reflect:Coll;

import Common.Math.Type;
import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Node = Common::Math::Type::Node;
    using Mesh = Resolved::Definitions::Type::Coll::Mesh;
    using Material = Resolved::Definitions::Type::Coll::Material;
    using Coll = Resolved::Definitions::Type::Coll::Coll;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Node>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Name", &Node::Name),
            MakeField("ParentIndex", &Node::ParentIndex),
            MakeField("NextSiblingIndex", &Node::NextSiblingIndex),
            MakeField("FirstChildIndex", &Node::FirstChildIndex),
        };
    };

    template <>
    struct Fields<Mesh>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("NodeIndex", &Mesh::NodeIndex),
            MakeField("RegionIndex", &Mesh::RegionIndex),
            MakeField("PermutationIndex", &Mesh::PermutationIndex),
            MakeField("Triangles", &Mesh::Triangles),
            MakeField("LocalMin", &Mesh::LocalMin),
            MakeField("LocalMax", &Mesh::LocalMax),
        };
    };

    template <>
    struct Fields<Material>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple {
            MakeField("Name", &Material::Name),
        };
    };

    template <>
    struct Fields<Coll>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Coll::TagName),
            MakeField("Nodes", &Coll::Nodes),
            MakeField("Meshes", &Coll::Meshes),
            MakeField("RegionNames", &Coll::RegionNames),
            MakeField("PermutationNames", &Coll::PermutationNames),
            MakeField("DefaultPermutationIndex", &Coll::DefaultPermutationIndex),
            MakeField("BoundsMin", &Coll::BoundsMin),
            MakeField("BoundsMax", &Coll::BoundsMax),
            MakeField("Materials", &Coll::Materials),
        };
    };
}