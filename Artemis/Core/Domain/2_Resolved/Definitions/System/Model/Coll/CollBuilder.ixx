export module Resolved.Definitions.System:Coll;

import Common.Math.Type;
import Map.Reader.Type;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
    // Builds the collision model definition.
    class CollBuilder
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using Node = Common::Math::Type::Node;
        using CollObject = Map::Tag::Type::Coll::Object::CollObject;
        using Coll_Regions_Permutations_BspsObject = Map::Tag::Type::Coll::Object::Coll_Regions_Permutations_BspsObject;
        using Coll = Resolved::Definitions::Type::Coll::Coll;

    public:
        CollBuilder() = default;
        ~CollBuilder() = default;

        // note: Triangulates every surface of every bsp, and computes the bounds of each mesh and of the model.
        auto Build(const CollObject& coll) -> Coll;

    private:
        auto BuildMeshes(const CollObject& coll, Coll& out) -> void;
        auto BuildNodes(const CollObject& coll, Coll& out) -> void;
        auto BuildMaterials(const CollObject& coll, Coll& out) -> void;
        auto BuildBounds(Coll& out) -> void;

        auto MakeVec3(const Map::Reader::Type::Structure::Primitive::Vec3& v) -> Vec3;

        auto CollectSurfaceVertexIndices(const Coll_Regions_Permutations_BspsObject& bsp,
            std::int32_t surfaceIndex) -> std::vector<std::int32_t>;
    };
}