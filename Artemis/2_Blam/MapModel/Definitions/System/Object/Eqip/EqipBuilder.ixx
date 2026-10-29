export module Blam.MapModel.Definitions.System:Eqip;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the equipment definition.
    class EqipBuilder
    {
    private:
        using EqipObject = Blam::Map::Tag::Type::Eqip::Object::EqipObject;
        using Eqip = Blam::MapModel::Definitions::Type::Eqip::Eqip;

    public:
        explicit EqipBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~EqipBuilder() = default;

        auto Build(const EqipObject& eqip) -> Eqip;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}