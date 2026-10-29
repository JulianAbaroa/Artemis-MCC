export module Blam.MapModel.Definitions.Reflect:Proj;

import :Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Proj::Proj;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Proj>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Proj::Base),
        };
    };
}