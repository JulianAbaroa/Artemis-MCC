export module Blam.MapModel.Definitions.Reflect:Coll;

import Common.Math.Type;
import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Common::Math::Type::Node;
    using Blam::MapModel::Definitions::Type::Coll::CollMesh;
    using Blam::MapModel::Definitions::Type::Coll::Material;
    using Blam::MapModel::Definitions::Type::Coll::Coll;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Node>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Name", &Node::Name),
            MakeField("ParentIndex", &Node::ParentIndex),
            MakeField("NextSiblingIndex", &Node::NextSiblingIndex),
            MakeField("FirstChildIndex", &Node::FirstChildIndex),
        };
    };

    template <>
    struct Fields<CollMesh>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("NodeIndex", &CollMesh::NodeIndex),
            MakeField("RegionIndex", &CollMesh::RegionIndex),
            MakeField("PermutationIndex", &CollMesh::PermutationIndex),
            MakeField("Triangles", &CollMesh::Triangles),
            MakeField("LocalMin", &CollMesh::LocalMin),
            MakeField("LocalMax", &CollMesh::LocalMax),
        };
    };

    template <>
    struct Fields<Material>
    {
        static constexpr auto k_Value = std::tuple {
            MakeField("Name", &Material::Name),
        };
    };

    template <>
    struct Fields<Coll>
    {
        static constexpr auto k_Value = std::tuple{
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