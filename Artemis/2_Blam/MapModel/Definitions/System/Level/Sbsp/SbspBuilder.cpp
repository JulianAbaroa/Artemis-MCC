module Blam.MapModel.Definitions.System;
import :Sbsp;

import Common.Math.System;
import Blam.Map.Reader.Type;

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Math::System::TransformPoint3x4;
    using Blam::Map::Reader::Type::Geometry::CollisionInstance;
    using Blam::Map::Tag::Type::Sbsp::Structure::SbspData;
}

namespace Blam::MapModel::Definitions::System
{
    auto SbspBuilder::Build(const SbspObject& sbsp, std::vector<Triangle> renderGeometry, SbspCollision collision) -> Sbsp
    {
        Sbsp out{};
        out.TagName = sbsp.TagName;

        this->BuildBounds(sbsp, out);

        out.RenderGeometry = std::move(renderGeometry);

        this->BuildCollision(sbsp, collision, out);

        return out;
    }

    auto SbspBuilder::BuildCollision(const SbspObject& sbsp, SbspCollision& collision, Sbsp& out) -> void
    {
        out.CollisionGeometry = std::move(collision.Structure);

        std::size_t placed{};
        for (const CollisionInstance& instance : collision.Instances)
        {
            placed += collision.Definitions[static_cast<std::size_t>(instance.DefinitionIndex)].size();
        }

        out.CollisionGeometry.reserve(out.CollisionGeometry.size() + placed);

        for (const CollisionInstance& instance : collision.Instances)
        {
            for (const Triangle& local : collision.Definitions[static_cast<std::size_t>(instance.DefinitionIndex)])
            {
                Triangle world{ local };
                world.A = TransformPoint3x4(local.A, instance.Matrix.data());
                world.B = TransformPoint3x4(local.B, instance.Matrix.data());
                world.C = TransformPoint3x4(local.C, instance.Matrix.data());

                out.CollisionGeometry.push_back(world);
            }
        }

        // The surfaces name their material by index into the collision materials of the bsp.
        const std::int16_t materialCount{ static_cast<std::int16_t>(sbsp.CollisionMaterials.size()) };

        for (Triangle& triangle : out.CollisionGeometry)
        {
            const bool isKnown{ triangle.Material >= 0 && triangle.Material < materialCount };
            triangle.Material = isKnown ? sbsp.CollisionMaterials[triangle.Material].GlobalMaterialIndex : std::int16_t{ -1 };
        }
    }

    auto SbspBuilder::BuildBounds(const SbspObject& sbsp, Sbsp& out) -> void
    {
        const SbspData& data = sbsp.Data;

        out.WorldBoundsMin = { data.WorldBoundsX.Min, data.WorldBoundsY.Min, data.WorldBoundsZ.Min };
        out.WorldBoundsMax = { data.WorldBoundsX.Max, data.WorldBoundsY.Max, data.WorldBoundsZ.Max };

        out.MoppBoundsMin = std::bit_cast<Vec3>(data.MoppBoundsMinimum);
        out.MoppBoundsMax = std::bit_cast<Vec3>(data.MoppBoundsMaximum);
    }
}