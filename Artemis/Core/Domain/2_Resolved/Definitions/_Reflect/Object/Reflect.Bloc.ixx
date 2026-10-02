export module Resolved.Definitions.Reflect:Bloc;

import :Object;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Bloc = Resolved::Definitions::Type::Bloc::Bloc;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Bloc>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Base", &Bloc::Base),
        };
    };
}