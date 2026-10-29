export module Blam.MapModel.Definitions.System:Vehi;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the vehicle definition.
    class VehiBuilder
    {
    private:
        using VehiObject = Blam::Map::Tag::Type::Vehi::Object::VehiObject;
        using Vehi = Blam::MapModel::Definitions::Type::Vehi::Vehi;

    public:
        explicit VehiBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~VehiBuilder() = default;

        auto Build(const VehiObject& vehi) -> Vehi;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}