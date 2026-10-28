module Resolved.Definitions.System;
import :Scen;

namespace Resolved::Definitions::System
{
    auto ScenBuilder::Build(const ScenObject& scen) -> Scen
    {
        Scen out{};

        out.Base = m_ObjectBuilder.Build(scen.TagName, scen.Data, scen.MultiplayerObject);

        return out;
    }
}