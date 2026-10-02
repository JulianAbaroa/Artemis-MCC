module Resolved.Definitions.System;
import :Mach;

import std;

namespace Resolved::Definitions::System
{
    auto MachBuilder::Build(const MachObject& mach) -> ResolvedMach
    {
        ResolvedMach out{};

        out.Base = m_ObjectBuilder.Build(mach.TagName, mach.Data, mach.MultiplayerObject);

        return out;
    }
}