export module Resolved.Definitions.System:Eqip;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    class EqipBuilder
    {
    private:
        using EqipObject = Map::Tag::Type::Eqip::Object::EqipObject;
        using ResolvedEqip = Resolved::Definitions::Type::Eqip::Eqip;

    public:
        EqipBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~EqipBuilder() = default;

        auto Build(const EqipObject& eqip) -> ResolvedEqip;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}