export module Resolved.World.System:ModelLink;

import Resolved.Definitions.Type;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
    // Builds the links between the collision model and the render model of a model tag.
    class ModelLinkBuilder
    {
    private:
        using Hlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using Coll = Resolved::Definitions::Type::Coll::Coll;
        using Mode = Resolved::Definitions::Type::Mode::Mode;
        using ModelLink = Resolved::World::Type::ModelLink::ModelLink;
        using Anchor = Resolved::World::Type::ModelLink::Anchor;

    public:
        ModelLinkBuilder() = default;
        ~ModelLinkBuilder() = default;

        // param coll: Null if the model has no collision model.
        // param mode: Null if the model has no render model.
        auto Build(const Hlmt& hlmt, const Coll* coll, const Mode* mode) const -> ModelLink;

    private:
        // Matches the nodes of both models by name.
        auto BuildNodeMap(const Coll* coll, const Mode* mode) const -> std::vector<std::int32_t>;

        // Matches the damage sections with the collision regions of the same name.
        auto BuildSectionToRegion(const Hlmt& hlmt, const Coll* coll) const -> std::vector<std::int32_t>;

        // Places each model target on the marker it names.
        auto BuildTargets(const Hlmt& hlmt, const Mode* mode) const -> std::vector<Anchor>;

        // Centers each collision region on the largest node of its default permutation.
        auto BuildRegionAnchors(const Coll* coll,
            const std::vector<std::int32_t>& nodeMap) const -> std::vector<Anchor>;

        // Centers the anchor on the bounds of the render model.
        auto BuildObjectCenter(const Mode* mode) const -> Anchor;
    };
}