export module Blam.MapModel.Definitions.Reflect:Ctrl;

import :Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Ctrl::Ctrl;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Ctrl>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Ctrl::Base),
        };
    };
}