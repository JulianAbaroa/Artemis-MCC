export module Resolved.Definitions.Reflect:Weap;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Weap::Weap;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Weap>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
            MakeField("Base", &Weap::Base),
        };
    };
}