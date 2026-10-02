module Resolved.Definitions.System;

import Common.Math.Type;
import Map.Reader.Type;

namespace
{
    namespace Magic = Map::Reader::Type::Magic;

    using Triangle = Common::Math::Type::Triangle;
    using SbspGeometry = Map::Reader::Type::Geometry::SbspGeometry;
    using TagTableEntry = Map::Reader::Type::Structure::TagTable::Entry;
    using ResolvedBipd = Resolved::Definitions::Type::Bipd::Bipd;
    using ResolvedBloc = Resolved::Definitions::Type::Bloc::Bloc;
    using ResolvedColl = Resolved::Definitions::Type::Coll::Coll;
    using ResolvedCtrl = Resolved::Definitions::Type::Ctrl::Ctrl;
    using ResolvedEqip = Resolved::Definitions::Type::Eqip::Eqip;
    using ResolvedHlmt = Resolved::Definitions::Type::Hlmt::Hlmt;
    using ResolvedJpt = Resolved::Definitions::Type::Jpt::Jpt;
    using ResolvedMach = Resolved::Definitions::Type::Mach::Mach;
    using ResolvedMode = Resolved::Definitions::Type::Mode::Mode;
    using ResolvedPhmo = Resolved::Definitions::Type::Phmo::Phmo;
    using ResolvedProj = Resolved::Definitions::Type::Proj::Proj;
    using ResolvedSbsp = Resolved::Definitions::Type::Sbsp::Sbsp;
    using ResolvedScen = Resolved::Definitions::Type::Scen::Scen;
    using ResolvedScnr = Resolved::Definitions::Type::Scnr::Scnr;
    using ResolvedVehi = Resolved::Definitions::Type::Vehi::Vehi;
    using ResolvedWeap = Resolved::Definitions::Type::Weap::Weap;
}

namespace Resolved::Definitions::System
{
    auto DefinitionsBuilder::BuildForMap() -> void
    {
        std::int32_t bipd{}, bloc{}, coll{}, ctrl{}, eqip{}, hlmt{}, jpt{}, mach{}, mode{}, phmo{}, proj{},
            sbsp{}, scen{}, scnr{}, vehi{}, weap{};

        std::vector<std::string> sbspTagNames;

        const std::int32_t tagCount = static_cast<std::int32_t>(
            m_TagIndexStore.GetTagsSize());

        for (std::int32_t i = 0; i < tagCount; ++i)
        {
            const TagTableEntry& entry = m_TagIndexStore.GetTag(i);
            if (entry.TagGroupIndex < 0) continue;

            const std::string tagName = m_TagIndexStore.GetTagName(i);
            if (tagName.empty()) continue;

            const std::uint32_t magic = m_TagIndexStore.GetGroupMagic(
                entry.TagGroupIndex);

            if (magic == Magic::Tag::k_Bipd)
            {
                if (!this->BuildBipd(tagName)) continue;
                ++bipd;
            }
            else if (magic == Magic::Tag::k_Bloc)
            {
                if (!this->BuildBloc(tagName)) continue;
                ++bloc;
            }
            else if (magic == Magic::Tag::k_Coll)
            {
                if (!this->BuildColl(tagName)) continue;
                ++coll;
            }
            else if (magic == Magic::Tag::k_Ctrl)
            {
                if (!this->BuildCtrl(tagName)) continue;
                ++ctrl;
            }
            else if (magic == Magic::Tag::k_Eqip)
            {
                if (!this->BuildEqip(tagName)) continue;
                ++eqip;
            }
            else if (magic == Magic::Tag::k_Hlmt)
            {
                if (!this->BuildHlmt(tagName)) continue;
                ++hlmt;
            }
            else if (magic == Magic::Tag::k_Jpt)
            {
                if (!this->BuildJpt(tagName)) continue;
                ++jpt;
            }
            else if (magic == Magic::Tag::k_Mach)
            {
                if (!this->BuildMach(tagName)) continue;
                ++mach;
            }
            else if (magic == Magic::Tag::k_Mode)
            {
                if (!this->BuildMode(tagName)) continue;
                ++mode;
            }
            else if (magic == Magic::Tag::k_Phmo)
            {
                if (!this->BuildPhmo(tagName)) continue;
                ++phmo;
            }
            else if (magic == Magic::Tag::k_Proj)
            {
                if (!this->BuildProj(tagName)) continue;
                ++proj;
            }
            else if (magic == Magic::Tag::k_Sbsp)
            {
                sbspTagNames.push_back(tagName);
            }
            else if (magic == Magic::Tag::k_Scen)
            {
                if (!this->BuildScen(tagName)) continue;
                ++scen;
            }
            else if (magic == Magic::Tag::k_Scnr)
            {
                if (!this->BuildScnr(tagName)) continue;
                ++scnr;
            }
            else if (magic == Magic::Tag::k_Vehi)
            {
                if (!this->BuildVehi(tagName)) continue;
                ++vehi;
            }
            else if (magic == Magic::Tag::k_Weap)
            {
                if (!this->BuildWeap(tagName)) continue;
                ++weap;
            }
        }

        sbsp = this->BuildSbsps(sbspTagNames);

        m_DefinitionsStore.Freeze();

        m_LogsService.Message("[DefinitionsBuilder] INFO: Definitions built."
            " Bipd: {} | Bloc: {} | Coll: {} | Ctrl: {} | Eqip: {} | Hlmt: {} | Jpt: {} | Mach: {} | Mode: {} |"
            " Phmo: {} | Proj: {} | Sbsp: {} | Scen: {} | Scnr: {} | Vehi: {} | Weap: {}",
            bipd, bloc, coll, ctrl, eqip, hlmt, jpt, mach, mode, phmo, proj, sbsp, scen, scnr, vehi, weap);
    }

    auto DefinitionsBuilder::BuildBipd(const std::string& tagName) -> bool
    {
        const BipdObject* bipd = m_TagCatalog.Bipd.Get(tagName);
        if (!bipd)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Bipd tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedBipd data = m_BipdBuilder.Build(*bipd);
        m_DefinitionsStore.AddResolvedBipd(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildBloc(const std::string& tagName) -> bool
    {
        const BlocObject* bloc = m_TagCatalog.Bloc.Get(tagName);
        if (!bloc)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Bloc tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedBloc data = m_BlocBuilder.Build(*bloc);
        m_DefinitionsStore.AddResolvedBloc(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildColl(const std::string& tagName) -> bool
    {
        const CollObject* coll = m_TagCatalog.Coll.Get(tagName);
        if (!coll)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Coll tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedColl data = m_CollBuilder.Build(*coll);
        m_DefinitionsStore.AddResolvedColl(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildCtrl(const std::string& tagName) -> bool
    {
        const CtrlObject* ctrl = m_TagCatalog.Ctrl.Get(tagName);
        if (!ctrl)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Ctrl tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedCtrl data = m_CtrlBuilder.Build(*ctrl);
        m_DefinitionsStore.AddResolvedCtrl(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildEqip(const std::string& tagName) -> bool
    {
        const EqipObject* eqip = m_TagCatalog.Eqip.Get(tagName);
        if (!eqip)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Eqip tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedEqip data = m_EqipBuilder.Build(*eqip);
        m_DefinitionsStore.AddResolvedEqip(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildHlmt(const std::string& tagName) -> bool
    {
        const HlmtObject* hlmt = m_TagCatalog.Hlmt.Get(tagName);
        if (!hlmt)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Hlmt tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedHlmt data = m_HlmtBuilder.Build(*hlmt);
        m_DefinitionsStore.AddResolvedHlmt(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildMode(const std::string& tagName) -> bool
    {
        const ModeObject* mode = m_TagCatalog.Mode.Get(tagName);
        if (!mode)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Mode tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedMode data = m_ModeBuilder.Build(*mode);
        m_DefinitionsStore.AddResolvedMode(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildPhmo(const std::string& tagName) -> bool
    {
        const PhmoObject* phmo = m_TagCatalog.Phmo.Get(tagName);
        if (!phmo)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Phmo tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedPhmo data = m_PhmoBuilder.Build(*phmo);
        m_DefinitionsStore.AddResolvedPhmo(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildJpt(const std::string& tagName) -> bool
    {
        const JptObject* jpt = m_TagCatalog.Jpt.Get(tagName);
        if (!jpt)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Jpt tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedJpt data = m_JptBuilder.Build(*jpt);
        m_DefinitionsStore.AddResolvedJpt(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildMach(const std::string& tagName) -> bool
    {
        const MachObject* mach = m_TagCatalog.Mach.Get(tagName);
        if (!mach)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Mach tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedMach data = m_MachBuilder.Build(*mach);
        m_DefinitionsStore.AddResolvedMach(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildProj(const std::string& tagName) -> bool
    {
        const ProjObject* proj = m_TagCatalog.Proj.Get(tagName);
        if (!proj)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Proj tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedProj data = m_ProjBuilder.Build(*proj);
        m_DefinitionsStore.AddResolvedProj(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildSbsps(std::vector<std::string>& tagNames) -> std::int32_t
    {
        if (tagNames.empty()) return 0;

        std::vector<SbspGeometry> rendered = m_GeometryLoaderService.ReadRenderGeometry(tagNames);

        std::unordered_map<std::string, std::vector<Triangle>> renderByName;
        renderByName.reserve(rendered.size());
        for (SbspGeometry& render : rendered)
        {
            renderByName.emplace(render.TagName, std::move(render.RenderGeometry));
        }

        if (renderByName.empty())
        {
            m_LogsService.Message("[DefinitionsBuilder] ERROR:"
                " Failed to read render geometry.");
        }
        else if (renderByName.size() < tagNames.size())
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Render geometry read for {} of {} sbsp.",
                static_cast<int>(renderByName.size()),
                static_cast<int>(tagNames.size()));
        }

        std::int32_t built{};

        for (const std::string& tagName : tagNames)
        {
            const SbspObject* sbsp = m_TagCatalog.Sbsp.Get(tagName);
            if (!sbsp)
            {
                m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                    " Sbsp tag found in table but not loaded: {}",
                    tagName.c_str());
                continue;
            }

            std::vector<Triangle> renderGeometry;
            const auto it = renderByName.find(tagName);
            if (it != renderByName.end())
            {
                renderGeometry = std::move(it->second);
            }

            ResolvedSbsp data = m_SbspBuilder.Build(*sbsp, std::move(renderGeometry));
            m_DefinitionsStore.AddResolvedSbsp(tagName, std::move(data));
            ++built;
        }

        return built;
    }

    auto DefinitionsBuilder::BuildScen(const std::string& tagName) -> bool
    {
        const ScenObject* scen = m_TagCatalog.Scen.Get(tagName);
        if (!scen)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Scen tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedScen data = m_ScenBuilder.Build(*scen);
        m_DefinitionsStore.AddResolvedScen(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildScnr(const std::string& tagName) -> bool
    {
        const ScnrObject* scnr = m_TagCatalog.Scnr.Get(tagName);
        if (!scnr)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Scnr tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedScnr data = m_ScnrBuilder.Build(*scnr);
        m_DefinitionsStore.AddResolvedScnr(tagName, std::move(data));
        return true;
    }


    auto DefinitionsBuilder::BuildVehi(const std::string& tagName) -> bool
    {
        const VehiObject* vehi = m_TagCatalog.Vehi.Get(tagName);
        if (!vehi)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Vehi tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedVehi data = m_VehiBuilder.Build(*vehi);
        m_DefinitionsStore.AddResolvedVehi(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::BuildWeap(const std::string& tagName) -> bool
    {
        const WeapObject* weap = m_TagCatalog.Weap.Get(tagName);
        if (!weap)
        {
            m_LogsService.Message("[DefinitionsBuilder] WARNING:"
                " Weap tag found in table but not loaded: {}",
                tagName.c_str());
            return false;
        }

        ResolvedWeap data = m_WeapBuilder.Build(*weap);
        m_DefinitionsStore.AddResolvedWeap(tagName, std::move(data));
        return true;
    }

    auto DefinitionsBuilder::Cleanup() -> void
    {
        m_DefinitionsStore.Cleanup();

        m_LogsService.Message("[DefinitionsBuilder] INFO: Cleanup completed.");
    }
}