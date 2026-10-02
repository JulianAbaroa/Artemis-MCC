module Resolved.Definitions.System;
import :Sbsp;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
    using SbspData = Map::Tag::Type::Sbsp::Structure::SbspData;
}

namespace Resolved::Definitions::System
{
    auto SbspBuilder::Build(const SbspObject& sbsp, std::vector<Triangle> renderGeometry) -> ResolvedSbsp
    {
        ResolvedSbsp out{};
        out.TagName = sbsp.TagName;

        this->BuildBounds(sbsp, out);

        out.RenderGeometry = std::move(renderGeometry);

        return out;
    }

    auto SbspBuilder::BuildBounds(const SbspObject& sbsp, ResolvedSbsp& out) -> void
    {
        const SbspData& d = sbsp.Data;

        out.WorldBoundsMin = { d.WorldBoundsX.Min, d.WorldBoundsY.Min, d.WorldBoundsZ.Min };
        out.WorldBoundsMax = { d.WorldBoundsX.Max, d.WorldBoundsY.Max, d.WorldBoundsZ.Max };

        out.MoppBoundsMin = std::bit_cast<Vec3>(d.MoppBoundsMinimum);
        out.MoppBoundsMax = std::bit_cast<Vec3>(d.MoppBoundsMaximum);
    }
}