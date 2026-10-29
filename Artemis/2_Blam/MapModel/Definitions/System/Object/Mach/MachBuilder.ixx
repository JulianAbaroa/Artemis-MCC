export module Blam.MapModel.Definitions.System:Mach;

import :Object;

import Blam.Map.Tag.Type;
import Blam.MapModel.Definitions.Type;

export namespace Blam::MapModel::Definitions::System
{
    // Builds the machine definition.
    class MachBuilder
    {
    private:
        using MachObject = Blam::Map::Tag::Type::Mach::Object::MachObject;
        using Mach = Blam::MapModel::Definitions::Type::Mach::Mach;

    public:
        explicit MachBuilder(ObjectBuilder& objectBuilder) :
            m_ObjectBuilder(objectBuilder) {}
        ~MachBuilder() = default;

        auto Build(const MachObject& mach) -> Mach;

    private:
        ObjectBuilder& m_ObjectBuilder;
    };
}