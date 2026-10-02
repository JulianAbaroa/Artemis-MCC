module Resolved.World.System;

import Resolved.World.Type;

namespace
{
    using ResolvedColl = Resolved::Definitions::Type::Coll::Coll;
    using ResolvedMode = Resolved::Definitions::Type::Mode::Mode;
}

namespace Resolved::World::System
{
    auto WorldBuilder::BuildForMap() -> void
    {
        this->LinkObjectColls();
        this->BuildModelLinks();

        m_WorldStore.Freeze();

        m_LogsService.Message("[WorldBuilder] INFO: World built.");
    }

    auto WorldBuilder::Cleanup() -> void
    {
        m_WorldStore.Cleanup();

        m_LogsService.Message("[WorldBuilder] INFO: Cleanup completed.");
    }

    auto WorldBuilder::LinkObjectColls() -> void
    {
        std::unordered_set<std::string> builtRegionStates;

        this->LinkObjectFamily(m_TagCatalog.Bipd, builtRegionStates);
        this->LinkObjectFamily(m_TagCatalog.Vehi, builtRegionStates);
        this->LinkObjectFamily(m_TagCatalog.Weap, builtRegionStates);
        this->LinkObjectFamily(m_TagCatalog.Eqip, builtRegionStates);
        this->LinkObjectFamily(m_TagCatalog.Scen, builtRegionStates);
        this->LinkObjectFamily(m_TagCatalog.Mach, builtRegionStates);
        this->LinkObjectFamily(m_TagCatalog.Ctrl, builtRegionStates);
        this->LinkObjectFamily(m_TagCatalog.Bloc, builtRegionStates);
    }

    auto WorldBuilder::BuildModelLinks() -> void
    {
        std::int32_t built = 0;
        std::int32_t withoutColl = 0;
        std::int32_t withoutMode = 0;

        for (const auto& [hlmtName, hlmt] : m_DefinitionsStore.GetAllResolvedHlmts())
        {
            if (hlmt.DamageSections.empty()) continue;

            const ResolvedColl* coll = hlmt.CollisionModelTagName.empty()
                ? nullptr : m_DefinitionsStore.GetResolvedColl(hlmt.CollisionModelTagName);
            const ResolvedMode* mode = hlmt.RenderModelTagName.empty()
                ? nullptr : m_DefinitionsStore.GetResolvedMode(hlmt.RenderModelTagName);

            if (!coll) ++withoutColl;
            if (!mode) ++withoutMode;

            m_WorldStore.AddResolvedModelLink(hlmtName,
                m_ModelLinkBuilder.Build(hlmt, coll, mode));
            ++built;
        }

        m_LogsService.Message("[WorldBuilder] INFO: Model links built."
            " Links: {} | Without coll: {} | Without mode: {}",
            built, withoutColl, withoutMode);
    }

    template <typename TObject>
    auto WorldBuilder::LinkObjectFamily(const TagStore<TObject>& state,
        std::unordered_set<std::string>& builtRegionStates) -> void
    {
        for (const auto& [objectTagName, object] : state.All())
        {
            const std::string hlmtName = m_TagResolverService.
                ResolveTagReferenceName(object.Data.Model);
            if (hlmtName.empty()) continue;

            const HlmtObject* hlmt = m_TagCatalog.Hlmt.Get(hlmtName);
            if (!hlmt) continue;

            m_WorldStore.LinkObjectHlmt(objectTagName, hlmtName);

            const std::string collName = m_TagResolverService.
                ResolveTagReferenceName(hlmt->Data.CollisionModel);
            if (collName.empty()) continue;

            m_WorldStore.LinkObjectColl(objectTagName, collName);

            if (builtRegionStates.insert(hlmtName).second)
            {
                this->BuildRegionStates(hlmtName, *hlmt, collName);
            }
        }
    }

    auto WorldBuilder::BuildRegionStates(const std::string& hlmtName,
        const HlmtObject& hlmt, const std::string& collName) -> void
    {
        const ResolvedColl* coll = m_DefinitionsStore.GetResolvedColl(collName);
        if (!coll) return;

        m_WorldStore.AddResolvedRegionStates(hlmtName,
            m_RegionStatesBuilder.Build(hlmt, *coll));
    }
}