export module Blam.MapModel.Definitions.System:Coll;

import Common.Math.Type;
import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;
import std;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the collision model definition.
    class CollBuilder
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;
        using Node = Common::Math::Type::Node;
        using CollObject = Blam::Map::Tag::Type::Coll::Object::CollObject;
        using Coll_Regions_Permutations_BspsObject = Blam::Map::Tag::Type::Coll::Object::Coll_Regions_Permutations_BspsObject;
        using Coll = Blam::MapModel::Definitions::Type::Coll::Coll;

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

        auto CollectSurfaceVertexIndices(const Coll_Regions_Permutations_BspsObject& bsp,
            std::int32_t surfaceIndex) -> std::vector<std::int32_t>;
    };
}