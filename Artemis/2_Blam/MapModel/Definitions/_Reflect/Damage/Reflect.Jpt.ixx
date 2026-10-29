export module Blam.MapModel.Definitions.Reflect:Jpt;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Jpt::Jpt;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Jpt>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("TagName", &Jpt::TagName),
            MakeField("Category", &Jpt::Category),
            MakeField("DamageFlags", &Jpt::DamageFlags),
            MakeField("DamageLowerBound", &Jpt::DamageLowerBound),
            MakeField("DamageUpperBoundMin", &Jpt::DamageUpperBoundMin),
            MakeField("DamageUpperBoundMax", &Jpt::DamageUpperBoundMax),
        };
    };
}