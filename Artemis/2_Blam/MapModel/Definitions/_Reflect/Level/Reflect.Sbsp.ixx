export module Blam.MapModel.Definitions.Reflect:Sbsp;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Sbsp::Sbsp;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Sbsp>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("TagName", &Sbsp::TagName),
            MakeField("WorldBoundsMin", &Sbsp::WorldBoundsMin),
            MakeField("WorldBoundsMax", &Sbsp::WorldBoundsMax),
            MakeField("MoppBoundsMin", &Sbsp::MoppBoundsMin),
            MakeField("MoppBoundsMax", &Sbsp::MoppBoundsMax),
            MakeField("RenderGeometry", &Sbsp::RenderGeometry),
        };
    };
}