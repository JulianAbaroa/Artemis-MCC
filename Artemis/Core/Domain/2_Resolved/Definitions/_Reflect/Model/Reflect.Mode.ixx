export module Resolved.Definitions.Reflect:Mode;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    namespace M = Resolved::Definitions::Type::Mode;
    using Marker = M::Marker;
    using MarkerGroup = M::MarkerGroup;
    using Bounds = M::Bounds;
    using Mode = M::Mode;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<Marker>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
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
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("NameId", &MarkerGroup::NameId),
            MakeField("Markers", &MarkerGroup::Markers),
        };
    };

    template <>
    struct Fields<Bounds>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("Min", &Bounds::Min),
            MakeField("Max", &Bounds::Max),
        };
    };

    template <>
    struct Fields<Mode>
    {
        static constexpr bool HasFields = true;
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Mode::TagName),
            MakeField("MarkerGroups", &Mode::MarkerGroups),
            MakeField("Nodes", &Mode::Nodes),
            MakeField("ModelBounds", &Mode::ModelBounds),
        };
    };
}
