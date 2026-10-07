module Resolved.Definitions.System;
import :Bipd;

namespace Resolved::Definitions::System
{
    auto BipdBuilder::Build(const BipdObject& bipd) -> Bipd
    {
        Bipd out{};

        out.Base = m_ObjectBuilder.Build(bipd.TagName, bipd.Data, bipd.MultiplayerObject);

        return out;
    }
}