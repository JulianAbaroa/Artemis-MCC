export module Blam.MapModel.Definitions.Reflect:Sddt;

import Common.Reflect.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Blam::MapModel::Definitions::Type::Sddt::SoftCeilingMesh;
    using Blam::MapModel::Definitions::Type::Sddt::Sddt;
}

export namespace Common::Reflect::Type
{
    template <>
    struct Fields<SoftCeilingMesh>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("Name", &SoftCeilingMesh::Name),
            MakeField("Kind", &SoftCeilingMesh::Kind),
            MakeField("Triangles", &SoftCeilingMesh::Triangles),
        };
    };

    template <>
    struct Fields<Sddt>
    {
        static constexpr auto k_Value = std::tuple{
            MakeField("TagName", &Sddt::TagName),
            MakeField("SoftCeilings", &Sddt::SoftCeilings),
        };
    };
}