module Resolved.World.System;

import Map.Reader.Type;
import Resolved.World.Type;

namespace
{
    namespace Magic = Map::Reader::Type::Magic;

    using TagTableEntry = Map::Reader::Type::Structure::TagTable::Entry;
    using SbspGeometry = Map::Reader::Type::Geometry::SbspGeometry;
    using ResolvedColl = Resolved::World::Type::Coll::Coll;
    using ResolvedMode = Resolved::World::Type::Mode::Mode;
}

namespace Resolved::World::System
{
    auto WorldBuilder::BuildForMap() -> void
    {
        std::vector<ResolvedSbsp> geometries;
        std::vector<const SbspObject*> sbspObjects;

        std::int32_t sbsp{}, coll{}, mode{};

        const std::int32_t tagCount = static_cast<std::int32_t>(
            m_TagIndexStore.GetTagsSize());

        for (std::int32_t i = 0; i < tagCount; ++i)
        {
            const TagTableEntry& entry = m_TagIndexStore.GetTag(i);
            if (entry.TagGroupIndex < 0) continue;

            const std::string tagName = m_TagIndexStore.GetTagName(i);
            if (tagName.empty()) continue;

            const std::uint32_t magic = m_TagIndexStore.
                GetGroupMagic(entry.TagGroupIndex);

            if (magic == Magic::Tag::k_Coll)
            {
                if (!this->BuildColl(tagName)) continue;
                ++coll;
            }
            else if (magic == Magic::Tag::k_Mode)
            {
                if (!this->BuildMode(tagName)) continue;
                ++mode;
            }
            else if (magic == Magic::Tag::k_Sbsp)
            {
                if (!this->BuildSbsp(tagName, geometries, sbspObjects)) continue;
                ++sbsp;
            }
        }

        if (sbsp > 1)
        {
            m_SbspSeamLinker.LinkSeams(geometries, sbspObjects);
        }

        if (!geometries.empty())
        {
            std::vector<std::string> sbspTagNames;
            sbspTagNames.reserve(geometries.size());
            for (const ResolvedSbsp& geometry : geometries)
            {
                sbspTagNames.push_back(geometry.TagName);
            }

            std::unordered_map<std::string_view, ResolvedSbsp*> byName;
            byName.reserve(geometries.size());
            for (ResolvedSbsp& geometry : geometries)
            {
                byName.emplace(geometry.TagName, &geometry);
            }

            std::vector<SbspGeometry> rendered =
                m_GeometryLoaderService.ReadRenderGeometry(sbspTagNames);

            std::size_t attached{};
            for (SbspGeometry& render : rendered)
            {
                const auto it = byName.find(render.TagName);
                if (it == byName.end()) continue;

                it->second->RenderGeometry = std::move(render.RenderGeometry);
                ++attached;
            }

            if (attached == 0)
            {
                m_LogsService.Message("[WorldBuilder] ERROR:"
                    " Failed to read render geometry.");
            }
            else if (attached < sbspTagNames.size())
            {
                m_LogsService.Message("[WorldBuilder] WARNING:"
                    " Render geometry read for {} of {} sbsp.",
                    static_cast<int>(attached),
                    static_cast<int>(sbspTagNames.size()));
            }
        }

        for (ResolvedSbsp& geometry : geometries)
        {
            m_WorldStore.AddResolvedSbsp(std::move(geometry));
        }

        this->LinkObjectColls();

        m_WorldStore.Freeze();

        m_LogsService.Message("[WorldBuilder] INFO: World built."
            " Coll: {} | Mode: {} | Sbsp: {}", coll, mode, sbsp);
    }

    auto WorldBuilder::BuildColl(const std::string& tagName) -> bool
    {
        const CollObject* coll = m_CollStore.Get(tagName);
        if (!coll)
        {
            m_LogsService.Message("[WorldBuilder] WARNING:"
                " Coll tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedColl geometry = m_CollBuilder.Build(*coll);
        m_WorldStore.AddResolvedColl(tagName, std::move(geometry));
        return true;
    }

    auto WorldBuilder::BuildMode(const std::string& tagName) -> bool
    {
        const ModeObject* mode = m_ModeStore.Get(tagName);
        if (!mode)
        {
            m_LogsService.Message("[WorldBuilder] WARNING:"
                " Mode tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedMode geometry = m_ModeBuilder.Build(*mode);
        m_WorldStore.AddResolvedMode(tagName, std::move(geometry));
        return true;
    }

    auto WorldBuilder::BuildSbsp(const std::string& tagName,
        std::vector<ResolvedSbsp>& geometries, 
        std::vector<const SbspObject*>& sbspObjects) -> bool
    {
        const SbspObject* sbsp = m_SbspStore.Get(tagName);
        if (!sbsp)
        {
            m_LogsService.Message("[WorldBuilder] WARNING:"
                " SBSP tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        std::int32_t sbspIndex = static_cast<std::int32_t>(geometries.size());
        ResolvedSbsp geometry = m_SbspBuilder.Build(*sbsp, sbspIndex);

        m_LogsService.Message("[WorldBuilder] INFO:"
            " Built {} clusters, {} portals, {} markers, {} seams.",
            geometry.Clusters.size(), geometry.Portals.size(),
            geometry.Markers.size(), sbsp->StructureSeams.size());

        geometries.push_back(std::move(geometry));
        sbspObjects.push_back(sbsp);

        return true;
    }

    auto WorldBuilder::Cleanup() -> void
    {
        m_WorldStore.Cleanup();

        m_LogsService.Message("[WorldBuilder] INFO: Cleanup completed.");
    }

    auto WorldBuilder::LinkObjectColls() -> void
    {
        std::unordered_set<std::string> builtRegionStates;

        this->LinkObjectFamily(m_BipdStore, builtRegionStates);
        this->LinkObjectFamily(m_VehiStore, builtRegionStates);
        this->LinkObjectFamily(m_WeapStore, builtRegionStates);
        this->LinkObjectFamily(m_EqipStore, builtRegionStates);
        this->LinkObjectFamily(m_ScenStore, builtRegionStates);
        this->LinkObjectFamily(m_MachStore, builtRegionStates);
        this->LinkObjectFamily(m_CtrlStore, builtRegionStates);
        this->LinkObjectFamily(m_BlocStore, builtRegionStates);
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

            const HlmtObject* hlmt = m_HlmtStore.Get(hlmtName);
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
        const ResolvedColl* coll = m_WorldStore.PeekResolvedColl(collName);
        if (!coll) return;

        m_WorldStore.AddResolvedRegionStates(hlmtName,
            m_RegionStatesBuilder.Build(hlmt, *coll));
    }
}