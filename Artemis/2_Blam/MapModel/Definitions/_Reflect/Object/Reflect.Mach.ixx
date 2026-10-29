export module Blam.MapModel.Definitions.Reflect:Mach;

import :Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Mach::Mach;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Mach>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Mach::Base),
        };
    };
}