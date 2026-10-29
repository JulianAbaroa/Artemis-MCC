export module Blam.MapModel.Definitions.Reflect:Mode;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Mode::Marker;
    using Blam::MapModel::Definitions::Type::Mode::MarkerGroup;
    using Blam::MapModel::Definitions::Type::Mode::Bounds;
    using Blam::MapModel::Definitions::Type::Mode::Mode;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Marker>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("NodeIndex", &Marker::NodeIndex),
            MakeField("Flags", &Marker::Flags),
            MakeField("Translation", &Marker::Translation),
            MakeField("Rotation", &Marker::Rotation),
            MakeField("Direction", &Marker::Direction),
            MakeField("Scale", &Marker::Scale),
        };
    };

    template <>
    struct Fields<MarkerGroup>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("NameId", &MarkerGroup::NameId),
            MakeField("Markers", &MarkerGroup::Markers),
        };
    };

    template <>
    struct Fields<Bounds>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Min", &Bounds::Min),
            MakeField("Max", &Bounds::Max),
        };
    };

    template <>
    struct Fields<Mode>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("TagName", &Mode::TagName),
            MakeField("MarkerGroups", &Mode::MarkerGroups),
            MakeField("Nodes", &Mode::Nodes),
            MakeField("ModelBounds", &Mode::ModelBounds),
        };
    };
}