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

        this->LinkObjectFamily(m_DefinitionsStore.Bipd, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.Bloc, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.Ctrl, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.Eqip, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.Mach, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.Proj, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.Scen, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.Vehi, builtRegionStates);
        this->LinkObjectFamily(m_DefinitionsStore.Weap, builtRegionStates);
    }

    template <typename TStore>
    auto WorldBuilder::LinkObjectFamily(const TStore& definitions,
        std::unordered_set<std::string>& builtRegionStates) -> void
    {
        for (const auto& [objectTagName, object] : definitions.All())
        {
            const std::string& hlmtName = object.Base.ModelTagName;
            if (hlmtName.empty()) continue;

            const Hlmt* hlmt = m_DefinitionsStore.Hlmt.Get(hlmtName);
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
        const Coll* coll = m_DefinitionsStore.Coll.Get(hlmt.CollisionModelTagName);
        if (!coll) return;

        m_WorldStore.AddRegionStates(hlmtName,
            m_RegionStatesBuilder.Build(hlmt, *coll));
    }

    auto WorldBuilder::BuildModelLinks() -> void
    {
        std::int32_t built{};
        std::int32_t withoutColl{};
        std::int32_t withoutMode{};

        for (const auto& [hlmtName, hlmt] : m_DefinitionsStore.Hlmt.All())
        {
            if (hlmt.DamageSections.empty()) continue;

            const Coll* coll = hlmt.CollisionModelTagName.empty()
                ? nullptr : m_DefinitionsStore.Coll.Get(hlmt.CollisionModelTagName);
            const Mode* mode = hlmt.RenderModelTagName.empty()
                ? nullptr : m_DefinitionsStore.Mode.Get(hlmt.RenderModelTagName);

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