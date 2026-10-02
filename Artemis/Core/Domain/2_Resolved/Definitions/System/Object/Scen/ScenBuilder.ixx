export module Resolved.Definitions.System:Scen;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    class ScenBuilder
    {
    private:
        using ScenObject = Map::Tag::Type::Scen::Object::ScenObject;
        using ResolvedScen = Resolved::Definitions::Type::Scen::Scen;

    public:
        ScenBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder){}
        ~ScenBuilder() = default;

        auto Build(const ScenObject& scen) -> ResolvedScen;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}