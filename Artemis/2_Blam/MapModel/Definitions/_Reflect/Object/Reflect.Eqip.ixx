export module Blam.MapModel.Definitions.Reflect:Eqip;

import :Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Eqip::Eqip;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Eqip>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Eqip::Base),
        };
    };
}