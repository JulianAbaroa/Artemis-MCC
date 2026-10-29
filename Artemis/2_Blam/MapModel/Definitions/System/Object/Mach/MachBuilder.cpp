module Blam.MapModel.Definitions.System;
import :Mach;

namespace Blam::MapModel::Definitions::System
{
    auto MachBuilder::Build(const MachObject& mach) -> Mach
    {
        Mach out{};

        out.Base = m_ObjectBuilder.Build(mach.TagName, mach.Data, mach.MultiplayerObject);

        return out;
    }
}