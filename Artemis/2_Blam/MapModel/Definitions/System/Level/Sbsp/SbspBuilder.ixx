export module Blam.MapModel.Definitions.System:Sbsp;

import Common.Math.Type;
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
        using Sbsp = Blam::MapModel::Definitions::Type::Sbsp::Sbsp;

    public:
        SbspBuilder() = default;
        ~SbspBuilder() = default;

        // param renderGeometry: Triangles of the render geometry, read apart from the tag.
        auto Build(const SbspObject& sbsp, std::vector<Triangle> renderGeometry) -> Sbsp;

    private:
        auto BuildBounds(const SbspObject& sbsp, Sbsp& out) -> void;
    };
}