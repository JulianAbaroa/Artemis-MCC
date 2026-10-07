export module Resolved.Definitions.Reflect:Eqip;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Eqip::Eqip;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Eqip>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
            MakeField("Base", &Eqip::Base),
        };
    };
}