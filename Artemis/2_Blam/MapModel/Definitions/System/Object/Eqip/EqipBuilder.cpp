module Blam.MapModel.Definitions.System;
import :Eqip;

namespace Blam::MapModel::Definitions::System
{
    auto EqipBuilder::Build(const EqipObject& eqip) -> Eqip
    {
        Eqip out{};

        out.Base = m_ObjectBuilder.Build(eqip.TagName, eqip.Data, eqip.MultiplayerObject);

        return out;
    }
}