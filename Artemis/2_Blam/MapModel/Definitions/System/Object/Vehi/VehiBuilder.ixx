export module Resolved.Definitions.System:Vehi;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the vehicle definition.
    class VehiBuilder
    {
    private:
        using VehiObject = Map::Tag::Type::Vehi::Object::VehiObject;
        using Vehi = Resolved::Definitions::Type::Vehi::Vehi;

    public:
        explicit VehiBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~VehiBuilder() = default;

        auto Build(const VehiObject& vehi) -> Vehi;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}