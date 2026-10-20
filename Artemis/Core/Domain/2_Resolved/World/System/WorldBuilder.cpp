module Resolved.World.System;

namespace Resolved::World::System
{
    auto WorldBuilder::BuildForMap() -> void
    {
        this->LinkObjects();
        this->BuildModelLinks();

        m_Raycaster.Build(m_DefinitionsStore);

        m_WorldStore.Freeze();

        m_LogsService.Message("[WorldBuilder] INFO: World built.");
    }

    auto WorldBuilder::Cleanup() -> void
    {
        m_Raycaster.Cleanup();
        m_WorldStore.Cleanup();

        m_LogsService.Message("[WorldBuilder] INFO: Cleanup completed.");
    }

    auto WorldBuilder::LinkObjects() -> void
    {
        std::unordered_set<std::string> builtRegionStates{};

        this->LinkObjectFamily(m_DefinitionsStore.m_Bipd, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.m_Bloc, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.m_Ctrl, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.m_Eqip, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.m_Mach, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.m_Proj, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.m_Scen, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.m_Vehi, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.m_Weap, builtRegionStates);
    }

    template <typename TStore>
    auto WorldBuilder::LinkObjectFamily(const TStore& definitions,
        std::unordered_set<std::string>& builtRegionStates) -> void
    {
        for (const auto& [objectTagName, object] : definitions.All())
        {
            const std::string& hlmtName = object.Base.ModelTagName;
            if (hlmtName.empty()) continue;

            const Hlmt* hlmt = m_DefinitionsStore.m_Hlmt.Get(hlmtName);
            if (!hlmt) continue;

            m_WorldStore.LinkObjectHlmt(objectTagName, hlmtName);

            if (hlmt->CollisionModelTagName.empty()) continue;

            m_WorldStore.LinkObjectColl(objectTagName, hlmt->CollisionModelTagName);

            if (builtRegionStates.insert(hlmtName).second)
            {
                this->BuildRegionStates(hlmtName, *hlmt);
            }
        }
    }

    auto WorldBuilder::BuildRegionStates(const std::string& hlmtName, const Hlmt& hlmt) -> void
    {
        const Coll* coll = m_DefinitionsStore.m_Coll.Get(hlmt.CollisionModelTagName);
        if (!coll) return;

        const Mode* mode = hlmt.RenderModelTagName.empty()
            ? nullptr : m_DefinitionsStore.m_Mode.Get(hlmt.RenderModelTagName);

        m_WorldStore.AddRegionStates(hlmtName,
            m_RegionStatesBuilder.Build(*coll, mode));
    }

    auto WorldBuilder::BuildModelLinks() -> void
    {
        std::int32_t built{};
        std::int32_t withoutColl{};
        std::int32_t withoutMode{};

        for (const auto& [hlmtName, hlmt] : m_DefinitionsStore.m_Hlmt.All())
        {
            if (hlmt.DamageSections.empty()) continue;

            const Coll* coll = hlmt.CollisionModelTagName.empty()
                ? nullptr : m_DefinitionsStore.m_Coll.Get(hlmt.CollisionModelTagName);
            const Mode* mode = hlmt.RenderModelTagName.empty()
                ? nullptr : m_DefinitionsStore.m_Mode.Get(hlmt.RenderModelTagName);

            if (!coll) ++withoutColl;
            if (!mode) ++withoutMode;

            m_WorldStore.AddModelLink(hlmtName,
                m_ModelLinkBuilder.Build(hlmt, coll, mode));
            ++built;
        }

        m_LogsService.Message("[WorldBuilder] INFO: Model links built."
            " Links: {} | Without coll: {} | Without mode: {}.",
            built, withoutColl, withoutMode);
    }
}