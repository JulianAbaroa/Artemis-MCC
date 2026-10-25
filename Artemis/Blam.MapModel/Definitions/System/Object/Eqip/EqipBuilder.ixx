export module Resolved.Definitions.System:Eqip;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the equipment definition.
    class EqipBuilder
    {
    private:
        using EqipObject = Map::Tag::Type::Eqip::Object::EqipObject;
        using Eqip = Resolved::Definitions::Type::Eqip::Eqip;

    public:
        explicit EqipBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~EqipBuilder() = default;

        auto Build(const EqipObject& eqip) -> Eqip;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}