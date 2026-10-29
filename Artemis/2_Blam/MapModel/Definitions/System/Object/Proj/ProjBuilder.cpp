module Blam.MapModel.Definitions.System;
import :Proj;

namespace Blam::MapModel::Definitions::System
{
    auto ProjBuilder::Build(const ProjObject& proj) -> Proj
    {
        Proj out{};

        out.Base = m_ObjectBuilder.Build(proj.TagName, proj.Data, proj.MultiplayerObject);

        return out;
    }
}