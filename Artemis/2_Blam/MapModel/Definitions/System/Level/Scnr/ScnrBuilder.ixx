export module Blam.MapModel.Definitions.System:Scnr;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the scenario definition.
    class ScnrBuilder
    {
    private:
        using ScnrObject = Blam::Map::Tag::Type::Scnr::Object::ScnrObject;
        using Scnr = Blam::MapModel::Definitions::Type::Scnr::Scnr;

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