export module Resolved.Definitions.Reflect:Ctrl;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Ctrl::Ctrl;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Ctrl>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
            MakeField("Base", &Ctrl::Base),
        };
    };
}