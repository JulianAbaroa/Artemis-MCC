export module Resolved.Definitions.System:Bloc;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    class BlocBuilder
    {
    private:
        using BlocObject = Map::Tag::Type::Bloc::Object::BlocObject;
        using ResolvedBloc = Resolved::Definitions::Type::Bloc::Bloc;

    public:
        explicit BlocBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~BlocBuilder() = default;

        auto Build(const BlocObject& bloc) -> ResolvedBloc;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}