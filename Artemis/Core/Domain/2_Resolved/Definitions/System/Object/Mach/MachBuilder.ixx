export module Resolved.Definitions.System:Mach;

import :Object;
import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    class MachBuilder
    {
    private:
        using MachObject = Map::Tag::Type::Mach::Object::MachObject;
        using ResolvedMach = Resolved::Definitions::Type::Mach::Mach;

    public:
        MachBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~MachBuilder() = default;

        auto Build(const MachObject& mach) -> ResolvedMach;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}