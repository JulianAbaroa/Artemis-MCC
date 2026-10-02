export module Resolved.Definitions.Reflect:Jpt;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Jpt = Resolved::Definitions::Type::Jpt::Jpt;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Jpt>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Jpt::TagName),
        };
    };
}