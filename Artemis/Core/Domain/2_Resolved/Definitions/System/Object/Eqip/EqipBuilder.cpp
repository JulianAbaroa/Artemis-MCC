module Resolved.Definitions.System;
import :Eqip;

import std;

namespace Resolved::Definitions::System
{
    auto EqipBuilder::Build(const EqipObject& eqip) -> ResolvedEqip
    {
        ResolvedEqip out{};

        out.Base = m_ObjectBuilder.Build(eqip.TagName, eqip.Data, eqip.MultiplayerObject);

        return out;
    }
}