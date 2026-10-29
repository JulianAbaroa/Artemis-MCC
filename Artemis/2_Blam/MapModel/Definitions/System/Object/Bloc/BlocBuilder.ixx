export module Blam.MapModel.Definitions.System:Bloc;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the crate definition.
    class BlocBuilder
    {
    private:
        using BlocObject = Blam::Map::Tag::Type::Bloc::Object::BlocObject;
        using Bloc = Blam::MapModel::Definitions::Type::Bloc::Bloc;

    public:
        explicit BlocBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~BlocBuilder() = default;

        auto Build(const BlocObject& bloc) -> Bloc;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}