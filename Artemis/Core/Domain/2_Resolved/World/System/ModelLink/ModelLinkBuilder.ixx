export module Resolved.World.System:ModelLink;

import Resolved.Definitions.Type;
import Resolved.World.Type;
import std;

export namespace Resolved::World::System
{
    class ModelLinkBuilder
    {
    private:
        using ResolvedHlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
        using ResolvedColl = Resolved::Definitions::Type::Coll::Coll;
        using ResolvedMode = Resolved::Definitions::Type::Mode::Mode;
        using ResolvedModelLink = Resolved::World::Type::ModelLink::ModelLink;
        using Anchor = Resolved::World::Type::ModelLink::Anchor;

    public:
        ModelLinkBuilder() = default;
        ~ModelLinkBuilder() = default;

        auto Build(const ResolvedHlmt& hlmt, const ResolvedColl* coll,
            const ResolvedMode* mode) const -> ResolvedModelLink;

    private:
        auto BuildNodeMap(const ResolvedColl* coll,
            const ResolvedMode* mode) const -> std::vector<std::int32_t>;

        auto BuildSectionToRegion(const ResolvedHlmt& hlmt,
            const ResolvedColl* coll) const -> std::vector<std::int32_t>;

        auto BuildTargets(const ResolvedHlmt& hlmt,
            const ResolvedMode* mode) const -> std::vector<Anchor>;

        auto BuildRegionAnchors(const ResolvedColl* coll,
            const std::vector<std::int32_t>& nodeMap) const -> std::vector<Anchor>;

        auto BuildObjectCenter(const ResolvedMode* mode) const -> Anchor;
    };
}