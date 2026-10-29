export module Blam.MapModel.Definitions.System:Ctrl;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the control definition.
    class CtrlBuilder
    {
    private:
        using CtrlObject = Blam::Map::Tag::Type::Ctrl::Object::CtrlObject;
        using Ctrl = Blam::MapModel::Definitions::Type::Ctrl::Ctrl;

    public:
        explicit CtrlBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~CtrlBuilder() = default;

        auto Build(const CtrlObject& ctrl) -> Ctrl;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}