export module Blam.MapModel.Definitions.System:Sddt;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the structure design definition.
    class SddtBuilder
    {
    private:
        using SddtObject = Blam::Map::Tag::Type::Sddt::Object::SddtObject;
        using Sddt = Blam::MapModel::Definitions::Type::Sddt::Sddt;

    public:
        SddtBuilder() = default;
        ~SddtBuilder() = default;

        auto Build(const SddtObject& sddt) -> Sddt;
    };
}