export module Resolved.Definitions.System:Weap;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the weapon definition.
    class WeapBuilder
    {
    private:
        using WeapObject = Map::Tag::Type::Weap::Object::WeapObject;
        using Weap = Resolved::Definitions::Type::Weap::Weap;

    public:
        explicit WeapBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~WeapBuilder() = default;

        auto Build(const WeapObject& weap) -> Weap;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}