module Blam.MapModel.Definitions.System;
import :Bipd;

namespace Blam::MapModel::Definitions::System
{
    auto BipdBuilder::Build(const BipdObject& bipd) -> Bipd
    {
        Bipd out{};

        out.Base = m_ObjectBuilder.Build(bipd.TagName, bipd.Data, bipd.MultiplayerObject);
        out.PhysicsFlags = bipd.Data.Flags_6;
        out.HeightStanding = bipd.Data.HeightStanding;
        out.HeightCrouching = bipd.Data.HeightCrouching;
        out.PhysicsRadius = bipd.Data.Radius;

        return out;
    }
}