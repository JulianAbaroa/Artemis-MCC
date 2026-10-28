export module Resolved.Definitions.System:Bipd;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the biped definition.
    class BipdBuilder
    {
    private:
        using BipdObject = Map::Tag::Type::Bipd::Object::BipdObject;
        using Bipd = Resolved::Definitions::Type::Bipd::Bipd;

    public:
        explicit BipdBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~BipdBuilder() = default;

        auto Build(const BipdObject& bipd) -> Bipd;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}