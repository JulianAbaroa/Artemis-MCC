module Blam.MapModel.Definitions.System;
import :Sbsp;

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::Map::Tag::Type::Sbsp::Structure::SbspData;
}

namespace Blam::MapModel::Definitions::System
{
    auto SbspBuilder::Build(const SbspObject& sbsp, std::vector<Triangle> renderGeometry) -> Sbsp
    {
        Sbsp out{};
        out.TagName = sbsp.TagName;

        this->BuildBounds(sbsp, out);

        out.RenderGeometry = std::move(renderGeometry);

        return out;
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