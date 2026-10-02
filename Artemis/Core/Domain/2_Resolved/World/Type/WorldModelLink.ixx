export module Resolved.World.Type:ModelLink;

import Common.Math.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
}

export namespace Resolved::World::Type::ModelLink
{
    // Where an anchor came from. Lets consumers judge how precise it is
    enum class AnchorSource : std::uint8_t
    {
        None,
        ModelTarget,
        HeadshotTarget,
        CollRegion,
        ObjectCenter,
    };

    // A point to aim at, expressed as (mode node, offset local to that node)
    // ModelNodeIndex == -1 means the offset is in object space (ObjectCenter)
    struct Anchor
    {
        std::int32_t ModelNodeIndex{ -1 };
        Vec3 LocalOffset{};

        float Radius{};
        float Relevance{};
        float ConeAngle{};

        // Section assigned by the hlmt ModelTarget (-1 if none). Only set for ModelTarget anchors
        std::int32_t SectionIndex{ -1 };

        bool Headshot{ false };

		AnchorSource Source{ AnchorSource::None };
    };

    // Correlation hlmt <-> coll <-> mode for one hlmt. Built once per map
    struct ModelLink
    {
        std::string CollTagName{};
		std::string ModeTagName{};

        // coll node index -> mode node index (by node stringid), -1 if not found
        std::vector<std::int32_t> CollNodeToModelNode{};

        // hlmt damage section index -> coll region index (by name), -1 if none
        std::vector<std::int32_t> SectionToRegion{};

        // One per hlmt ModelTarget (same index). Source == None if the marker did not resolve
        std::vector<Anchor> Targets{};

        // One per coll region (same index). Source == None if the region has no usable geometry
        std::vector<Anchor> RegionAnchors{};

        // Center of the render model bounds. Source == None if no mode
		Anchor ObjectCenter{};
    };
}