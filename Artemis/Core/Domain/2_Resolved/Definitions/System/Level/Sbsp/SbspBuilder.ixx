export module Resolved.Definitions.System:Sbsp;

import Common.Math.Type;
import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
    class SbspBuilder
    {
    private:
        using Triangle = Common::Math::Type::Triangle;
        using SbspObject = Map::Tag::Type::Sbsp::Object::SbspObject;
        using ResolvedSbsp = Resolved::Definitions::Type::Sbsp::Sbsp;

    public:
        SbspBuilder() = default;
        ~SbspBuilder() = default;

        auto Build(const SbspObject& sbsp, std::vector<Triangle> renderGeometry) -> ResolvedSbsp;

    private:
        auto BuildBounds(const SbspObject& sbsp, ResolvedSbsp& out) -> void;
    };
}