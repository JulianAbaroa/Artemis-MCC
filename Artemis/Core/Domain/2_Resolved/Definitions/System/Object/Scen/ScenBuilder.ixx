export module Resolved.Definitions.System:Scen;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the scenery definition.
    class ScenBuilder
    {
    private:
        using ScenObject = Map::Tag::Type::Scen::Object::ScenObject;
        using Scen = Resolved::Definitions::Type::Scen::Scen;

    public:
        explicit ScenBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~ScenBuilder() = default;

        auto Build(const ScenObject& scen) -> Scen;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}