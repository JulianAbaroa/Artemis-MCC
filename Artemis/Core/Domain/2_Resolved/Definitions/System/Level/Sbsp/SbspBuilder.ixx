export module Resolved.Definitions.System:Sbsp;

import Common.Math.Type;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
    // Builds the structure bsp definition.
    class SbspBuilder
    {
    private:
        using Triangle = Common::Math::Type::Triangle;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using Sbsp = Resolved::Definitions::Type::Sbsp::Sbsp;

    public:
        SbspBuilder() = default;
        ~SbspBuilder() = default;

        // param renderGeometry: Triangles of the render geometry, read apart from the tag.
        auto Build(const SbspObject& sbsp, std::vector<Triangle> renderGeometry) -> Sbsp;

    private:
        auto BuildBounds(const SbspObject& sbsp, Sbsp& out) -> void;
    };
}