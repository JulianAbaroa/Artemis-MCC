export module Blam.MapModel.Definitions.System:Scen;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the scenery definition.
    class ScenBuilder
    {
    private:
        using ScenObject = Blam::Map::Tag::Type::Scen::Object::ScenObject;
        using Scen = Blam::MapModel::Definitions::Type::Scen::Scen;

    public:
        explicit ScenBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~ScenBuilder() = default;

        auto Build(const ScenObject& scen) -> Scen;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}