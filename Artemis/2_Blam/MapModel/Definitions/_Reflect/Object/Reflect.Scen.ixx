export module Blam.MapModel.Definitions.Reflect:Scen;

import :Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Scen::Scen;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Scen>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Scen::Base),
        };
    };
}