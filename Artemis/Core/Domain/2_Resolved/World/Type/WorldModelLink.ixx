export module Resolved.World.Type:ModelLink;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace Resolved::World::Type::ModelLink
{
    // How an anchor was found.
    enum class AnchorSource : std::uint8_t
    {
        None,
        ModelTarget,
        HeadshotTarget,
        CollRegion,
        ObjectCenter,
    };

    // Point of a render model, in the space of one of its nodes.
    struct Anchor
    {
        std::int32_t ModelNodeIndex{ -1 };
        Vec3 LocalOffset{};

        float Radius{};
        float Relevance{};
        float ConeAngle{};

        std::int32_t SectionIndex{ -1 };

        bool Headshot{};

        AnchorSource Source{ AnchorSource::None };
    };

    // Links the collision model and the render model of one model tag.
    struct ModelLink
    {
        std::string CollTagName{};
        std::string ModeTagName{};

        // Render model node of each collision model node, or -1.
        std::vector<std::int32_t> CollNodeToModelNode{};

        // Collision region of each damage section, or -1.
        std::vector<std::int32_t> SectionToRegion{};

        // Anchors of the model targets, in the same order.
        std::vector<Anchor> Targets{};

        // Anchor of each collision region.
        std::vector<Anchor> RegionAnchors{};

        Anchor ObjectCenter{};
    };
}