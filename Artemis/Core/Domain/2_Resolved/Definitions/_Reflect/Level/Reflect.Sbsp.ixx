export module Resolved.Definitions.Reflect:Sbsp;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Sbsp = Resolved::Definitions::Type::Sbsp::Sbsp;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Sbsp>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Sbsp::TagName),
            MakeField("WorldBoundsMin", &Sbsp::WorldBoundsMin),
            MakeField("WorldBoundsMax", &Sbsp::WorldBoundsMax),
            MakeField("MoppBoundsMin", &Sbsp::MoppBoundsMin),
            MakeField("MoppBoundsMax", &Sbsp::MoppBoundsMax),
            MakeField("RenderGeometry", &Sbsp::RenderGeometry),
        };
    };
}