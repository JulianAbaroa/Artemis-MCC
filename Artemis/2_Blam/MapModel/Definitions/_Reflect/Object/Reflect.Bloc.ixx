export module Blam.MapModel.Definitions.Reflect:Bloc;

import :Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Bloc::Bloc;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Bloc>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Bloc::Base),
        };
    };
}