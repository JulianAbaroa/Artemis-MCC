export module Resolved.Definitions.System:Scnr;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the scenario definition.
    class ScnrBuilder
    {
    private:
        using ScnrObject = Map::Tag::Type::Scnr::Object::ScnrObject;
        using Scnr = Resolved::Definitions::Type::Scnr::Scnr;

    public:
        ScnrBuilder() = default;
        ~ScnrBuilder() = default;

        auto Build(const ScnrObject& scnr) -> Scnr;

    private:
        static auto BuildTriggerVolumes(const ScnrObject& scnr, Scnr& out) -> void;
        static auto BuildBoundaryTriggers(const ScnrObject& scnr, Scnr& out) -> void;
        static auto BuildSoftCeilings(const ScnrObject& scnr, Scnr& out) -> void;
    };
}