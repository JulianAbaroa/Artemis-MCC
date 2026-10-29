export module Blam.MapModel.Definitions.Reflect:Bipd;

import :Object;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Bipd::Bipd;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Bipd>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Base", &Bipd::Base),
            MakeField("PhysicsFlags", &Bipd::PhysicsFlags),
            MakeField("HeightStanding", &Bipd::HeightStanding),
            MakeField("HeightCrouching", &Bipd::HeightCrouching),
            MakeField("PhysicsRadius", &Bipd::PhysicsRadius),
        };
    };
}