export module Blam.MapModel.Definitions.System:Sbsp;

import Common.Math.Type;
import Blam.Map.Reader.Type;
import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;
import std;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the structure bsp definition.
    class SbspBuilder
    {
    private:
        using Triangle = Common::Math::Type::Triangle;
        using SbspObject = Blam::Map::Tag::Type::Sbsp::Object::SbspObject;
        using SbspCollision = Blam::Map::Reader::Type::Geometry::SbspCollision;
        using Sbsp = Blam::MapModel::Definitions::Type::Sbsp::Sbsp;

    public:
        SbspBuilder() = default;
        ~SbspBuilder() = default;

        // param renderGeometry: Triangles of the render geometry, read apart from the tag.
        // param collision: Collision of the bsp, read apart from the tag. The placements are applied here.
        auto Build(const SbspObject& sbsp, std::vector<Triangle> renderGeometry, SbspCollision collision) -> Sbsp;

    private:
        auto BuildBounds(const SbspObject& sbsp, Sbsp& out) -> void;
        auto BuildCollision(const SbspObject& sbsp, SbspCollision& collision, Sbsp& out) -> void;
    };
}