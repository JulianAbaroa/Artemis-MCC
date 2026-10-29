export module Blam.MapModel.World.System:ModelLink;

import Blam.MapModel.Definitions.Type;
import Blam.MapModel.World.Type;
import std;

export namespace Blam::MapModel::World::System
{
    // Builds the links between the collision model and the render model of a model tag.
    class ModelLinkBuilder
    {
    private:
        using Hlmt = Blam::MapModel::Definitions::Type::Hlmt::Hlmt;
        using Coll = Blam::MapModel::Definitions::Type::Coll::Coll;
        using Mode = Blam::MapModel::Definitions::Type::Mode::Mode;
        using ModelLink = Blam::MapModel::World::Type::ModelLink::ModelLink;
        using ModelLinkAnchor = Blam::MapModel::World::Type::ModelLink::ModelLinkAnchor;

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
        auto BuildTargets(const Hlmt& hlmt, const Mode* mode) const -> std::vector<ModelLinkAnchor>;

        // Centers each collision region on the largest node of its default permutation.
        auto BuildRegionAnchors(const Coll* coll,
            const std::vector<std::int32_t>& nodeMap) const -> std::vector<ModelLinkAnchor>;

        // Centers the anchor on the bounds of the render model.
        auto BuildObjectCenter(const Mode* mode) const -> ModelLinkAnchor;
    };
}