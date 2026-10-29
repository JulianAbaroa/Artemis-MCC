export module Blam.MapModel.Definitions.System:Weap;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the weapon definition.
    class WeapBuilder
    {
    private:
        using WeapObject = Blam::Map::Tag::Type::Weap::Object::WeapObject;
        using Weap = Blam::MapModel::Definitions::Type::Weap::Weap;

    public:
        explicit WeapBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~WeapBuilder() = default;

        auto Build(const WeapObject& weap) -> Weap;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}