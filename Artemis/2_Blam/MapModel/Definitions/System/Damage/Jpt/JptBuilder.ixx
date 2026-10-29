export module Blam.MapModel.Definitions.System:Jpt;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the damage effect definition.
    // Waiting for some system to need this.
    class JptBuilder
    {
    private:
        using JptObject = Blam::Map::Tag::Type::Jpt::Object::JptObject;
        using Jpt = Blam::MapModel::Definitions::Type::Jpt::Jpt;

    public:
        JptBuilder() = default;
        ~JptBuilder() = default;

        auto Build(const JptObject& jpt) -> Jpt;
    };
}