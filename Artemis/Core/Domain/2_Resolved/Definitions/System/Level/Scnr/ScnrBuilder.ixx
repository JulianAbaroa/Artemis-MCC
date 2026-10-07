export module Resolved.Definitions.System:Scnr;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the scenario definition.
    // Waiting for some system to need this.
    class ScnrBuilder
    {
    private:
        using ScnrObject = Map::Tag::Type::Scnr::Object::ScnrObject;
        using Scnr = Resolved::Definitions::Type::Scnr::Scnr;

    public:
        ScnrBuilder() = default;
        ~ScnrBuilder() = default;

        auto Build(const ScnrObject& scnr) -> Scnr;
    };
}