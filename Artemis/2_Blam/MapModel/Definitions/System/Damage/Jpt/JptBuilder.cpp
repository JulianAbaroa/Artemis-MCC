module Blam.MapModel.Definitions.System;
import :Jpt;

namespace Blam::MapModel::Definitions::System
{
    auto JptBuilder::Build(const JptObject& jpt) -> Jpt
    {
        Jpt out{};

        out.TagName = jpt.TagName;
        out.RadiusMin = jpt.Data.Radius.Min;
        out.RadiusMax = jpt.Data.Radius.Max;
        out.Category = jpt.Data.Category;
        out.DamageFlags = jpt.Data.Flags_2;
        out.DamageLowerBound = jpt.Data.DamageLowerBound;
        out.DamageUpperBoundMin = jpt.Data.DamageUpperBound.Min;
        out.DamageUpperBoundMax = jpt.Data.DamageUpperBound.Max;

        return out;
    }
}