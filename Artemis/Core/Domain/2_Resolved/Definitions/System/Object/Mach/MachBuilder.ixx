export module Resolved.Definitions.System:Mach;

import :Object;

import Map.Tag.Type;
import Resolved.Definitions.Type;

export namespace Resolved::Definitions::System
{
    // Builds the machine definition.
    class MachBuilder
    {
    private:
        using MachObject = Map::Tag::Type::Mach::Object::MachObject;
        using Mach = Resolved::Definitions::Type::Mach::Mach;

    public:
        explicit MachBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~MachBuilder() = default;

        auto Build(const MachObject& mach) -> Mach;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}