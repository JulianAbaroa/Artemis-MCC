module Resolved.Definitions.System;
import :Scen;

import std;

namespace Resolved::Definitions::System
{
    auto ScenBuilder::Build(const ScenObject& scen) -> ResolvedScen
    {
        ResolvedScen out{};

        out.Base = m_ObjectBuilder.Build(scen.TagName, scen.Data, scen.MultiplayerObject);

        return out;
    }
}