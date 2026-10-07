export module Resolved.Definitions.System:Bloc;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the crate definition.
    class BlocBuilder
    {
    private:
        using BlocObject = Map::Tag::Type::Bloc::Object::BlocObject;
        using Bloc = Resolved::Definitions::Type::Bloc::Bloc;

    public:
        explicit BlocBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~BlocBuilder() = default;

        auto Build(const BlocObject& bloc) -> Bloc;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}