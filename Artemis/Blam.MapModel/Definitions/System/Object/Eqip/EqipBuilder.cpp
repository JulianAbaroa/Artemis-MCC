module Resolved.Definitions.System;
import :Eqip;

namespace Resolved::Definitions::System
{
    auto EqipBuilder::Build(const EqipObject& eqip) -> Eqip
    {
        Eqip out{};

        out.Base = m_ObjectBuilder.Build(eqip.TagName, eqip.Data, eqip.MultiplayerObject);

        return out;
    }
}