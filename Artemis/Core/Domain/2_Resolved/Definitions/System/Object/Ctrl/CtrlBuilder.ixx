export module Resolved.Definitions.System:Ctrl;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    class CtrlBuilder
    {
    private:
        using CtrlObject = Map::Tag::Type::Ctrl::Object::CtrlObject;
        using ResolvedCtrl = Resolved::Definitions::Type::Ctrl::Ctrl;

    public:
        CtrlBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~CtrlBuilder() = default;

        auto Build(const CtrlObject& ctrl) -> ResolvedCtrl;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}