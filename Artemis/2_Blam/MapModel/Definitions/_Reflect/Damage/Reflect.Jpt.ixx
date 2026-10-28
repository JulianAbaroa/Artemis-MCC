export module Resolved.Definitions.Reflect:Jpt;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Jpt::Jpt;
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