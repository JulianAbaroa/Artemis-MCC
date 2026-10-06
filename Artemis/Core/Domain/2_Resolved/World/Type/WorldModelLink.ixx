export module Resolved.World.Type:ModelLink;

import Common.Math.Type;
import std;

namespace
{
    using Vec3 = Common::Math::Type::Vec3;
}

export namespace Resolved::World::Type::ModelLink
{
    struct NodeBounds
    {
        bool Used{ false };
        float MinX{}, MinY{}, MinZ{}, MaxX{}, MaxY{}, MaxZ{};
    };

    enum class AnchorSource : std::uint8_t
    {
        None,
        ModelTarget,
        HeadshotTarget,
        CollRegion,
        ObjectCenter,
    };

    struct Anchor
    {
        std::int32_t ModelNodeIndex{ -1 };
        Vec3 LocalOffset{};

        float Radius{};
        float Relevance{};
        float ConeAngle{};

        std::int32_t SectionIndex{ -1 };

        bool Headshot{ false };

		AnchorSource Source{ AnchorSource::None };
    };

    struct ModelLink
    {
        std::string CollTagName{};
		std::string ModeTagName{};

        std::vector<std::int32_t> CollNodeToModelNode{};
        std::vector<std::int32_t> SectionToRegion{};
        std::vector<Anchor> Targets{};
        std::vector<Anchor> RegionAnchors{};
		Anchor ObjectCenter{};
    };
}