export module Resolved.Definitions.System:Scnr;

import Map.Tag.Type;
import Resolved.Definitions.Type;
import std;

export namespace Resolved::Definitions::System
{
    class ScnrBuilder
    {
    private:
        using ScnrObject = Map::Tag::Type::Scnr::Object::ScnrObject;
        using ResolvedScnr = Resolved::Definitions::Type::Scnr::Scnr;

    public:
        ScnrBuilder() = default;
        ~ScnrBuilder() = default;

        auto Build(const ScnrObject& scnr) -> ResolvedScnr;
    };
}