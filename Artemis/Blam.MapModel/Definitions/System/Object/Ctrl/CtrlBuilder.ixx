export module Resolved.Definitions.System:Ctrl;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the control definition.
    class CtrlBuilder
    {
    private:
        using CtrlObject = Map::Tag::Type::Ctrl::Object::CtrlObject;
        using Ctrl = Resolved::Definitions::Type::Ctrl::Ctrl;

    public:
        explicit CtrlBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~CtrlBuilder() = default;

        auto Build(const CtrlObject& ctrl) -> Ctrl;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}