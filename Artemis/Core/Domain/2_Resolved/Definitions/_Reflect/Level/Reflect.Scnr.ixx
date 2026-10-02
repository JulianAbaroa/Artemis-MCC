export module Resolved.Definitions.Reflect:Scnr;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Scnr = Resolved::Definitions::Type::Scnr::Scnr;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Scnr>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Scnr::TagName),
        };
    };
}