export module Resolved.Definitions.Reflect:Sddt;

import Common.Reflect.Type;
import Resolved.Definitions.Type;
import std;

namespace
{
    using Resolved::Definitions::Type::Sddt::SoftCeilingMesh;
    using Resolved::Definitions::Type::Sddt::Sddt;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<SoftCeilingMesh>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
            MakeField("Name", &SoftCeilingMesh::Name),
            MakeField("Kind", &SoftCeilingMesh::Kind),
            MakeField("Triangles", &SoftCeilingMesh::Triangles),
        };
    };

    template <>
    struct Fields<Sddt>
    {
        static constexpr bool HasFields{ true };
        static constexpr auto Value = std::tuple{
            MakeField("TagName", &Sddt::TagName),
            MakeField("SoftCeilings", &Sddt::SoftCeilings),
        };
    };
}