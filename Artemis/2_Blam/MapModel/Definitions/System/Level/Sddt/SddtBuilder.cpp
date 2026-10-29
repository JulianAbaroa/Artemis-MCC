module Blam.MapModel.Definitions.System;
import :Sddt;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;

namespace
{
    using Common::Math::Type::Triangle;
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Scnr::SoftCeilingKind;
    using Blam::MapModel::Definitions::Type::Sddt::SoftCeilingMesh;

    using Blam::MapModel::Definitions::Type::Scnr::k_LastSoftCeilingKind;

    template <typename TVec3>
    constexpr auto ToVec3(const TVec3& value) -> Vec3
    {
        return { value.X, value.Y, value.Z };
    }
}

namespace Blam::MapModel::Definitions::System
{
    auto SddtBuilder::Build(const SddtObject& sddt) -> Sddt
    {
        Sddt out{};

        out.TagName = sddt.TagName;

        out.SoftCeilings.reserve(sddt.SoftCeilings.size());

        for (const auto& ceiling : sddt.SoftCeilings)
        {
            SoftCeilingMesh mesh{};

            mesh.Name = ceiling.Name;
            mesh.Kind = ceiling.Type <= k_LastSoftCeilingKind
                ? static_cast<SoftCeilingKind>(ceiling.Type)
                : SoftCeilingKind::Invalid;

            mesh.Triangles.reserve(ceiling.SoftCeilingTriangles.size());

            for (const auto& triangle : ceiling.SoftCeilingTriangles)
            {
                Triangle built{};

                built.A = ToVec3(triangle.Vertex0);
                built.B = ToVec3(triangle.Vertex1);
                built.C = ToVec3(triangle.Vertex2);

                mesh.Triangles.push_back(built);
            }

            out.SoftCeilings.push_back(std::move(mesh));
        }

        return out;
    }
}