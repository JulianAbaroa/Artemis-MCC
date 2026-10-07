module Resolved.Definitions.System;
import :Proj;

namespace Resolved::Definitions::System
{
    auto ProjBuilder::Build(const ProjObject& proj) -> Proj
    {
        Proj out{};

        out.Base = m_ObjectBuilder.Build(proj.TagName, proj.Data, proj.MultiplayerObject);

        return out;
    }
}