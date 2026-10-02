module Resolved.Definitions.System;
import :Bipd;

import :Object;
import std;

namespace Resolved::Definitions::System
{
    auto BipdBuilder::Build(const BipdObject& bipd) -> ResolvedBipd
    {
        ResolvedBipd out{};

        out.Base = m_ObjectBuilder.Build(bipd.TagName, bipd.Data, bipd.MultiplayerObject);

        return out;
	}
}