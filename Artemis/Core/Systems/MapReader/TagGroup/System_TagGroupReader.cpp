#include "pch.h"

#include "System_TagGroupReader.h"

#include "Core/Types/Map/MapMagics.h"

#include "Core/States/MapReader/State_MapReader.h"
#include "Core/States/MapReader/Phmo/State_MapPhmo.h"
#include "Core/States/MapReader/Coll/State_MapColl.h"
#include "Core/States/MapReader/Mode/State_MapMode.h"
#include "Core/States/MapReader/Vehi/State_MapVehi.h"
#include "Core/States/MapReader/Bloc/State_MapBloc.h"
#include "Core/States/MapReader/Sbsp/State_MapSbsp.h"
#include "Core/States/MapReader/Weap/State_MapWeap.h"
#include "Core/States/MapReader/Proj/State_MapProj.h"
#include "Core/States/MapReader/Bipd/State_MapBipd.h"
#include "Core/States/MapReader/Eqip/State_MapEqip.h"
#include "Core/States/MapReader/Scen/State_MapScen.h"
#include "Core/States/MapReader/Scnr/State_MapScnr.h"
#include "Core/States/MapReader/Jmad/State_MapJmad.h"
#include "Core/States/MapReader/Ctrl/State_MapCtrl.h"
#include "Core/States/MapReader/Mach/State_MapMach.h"
#include "Core/States/MapReader/Zone/State_MapZone.h"
#include "Core/States/MapReader/Play/State_MapPlay.h"
#include "Core/States/MapReader/Sldt/State_MapSldt.h"
#include "Core/States/MapReader/Lbsp/State_MapLbsp.h"

#include "../System_MapReader.h"
#include "TagGroupReader.h"
#include "Core/Systems/Logs/System_Logs.h"

#include "Generated/Hlmt/HlmtOffsets.h" 

void System_TagGroupReader::LoadForMap()
{
    FILE* file = m_Deps.System_Header.OpenMapFile();
    if (!file)
    {
        m_Deps.System_Logs.Log("[MapTagGroupSystem] ERROR:"
            " Could not open map file.");
        return;
    }

    TagGroupReader reader(m_Deps.System_Header);

    this->LoadHlmtFamily(file, reader);
    this->LoadDirectTags(file, reader);

    fclose(file);
    m_Deps.System_Logs.Log("[MapTagGroupSystem] INFO:"
        " Load completed.");
}

void System_TagGroupReader::Cleanup()
{
    // Hlmt-derived.
    m_Deps.State_MapPhmo.Cleanup();
    m_Deps.State_MapColl.Cleanup();
    m_Deps.State_MapMode.Cleanup();
    m_Deps.State_MapJmad.Cleanup();

    // Direct TagTable entry.
    m_Deps.State_MapVehi.Cleanup();
    m_Deps.State_MapBloc.Cleanup();
    m_Deps.State_MapSbsp.Cleanup();
    m_Deps.State_MapWeap.Cleanup();
    m_Deps.State_MapProj.Cleanup();
    m_Deps.State_MapBipd.Cleanup();
    m_Deps.State_MapEqip.Cleanup();
    m_Deps.State_MapScen.Cleanup();
    m_Deps.State_MapScnr.Cleanup();
    m_Deps.State_MapCtrl.Cleanup();
    m_Deps.State_MapZone.Cleanup();
    m_Deps.State_MapSldt.Cleanup();
    m_Deps.State_MapLbsp.Cleanup();

    m_Deps.System_Logs.Log("[MapTagGroupSystem] INFO:"
        " Cleanup completed.");
}

void System_TagGroupReader::LoadHlmtFamily(FILE* file, TagGroupReader& reader)
{
    const int32_t tagCount = (int32_t)m_Deps.State_Map.GetTagsSize();
    
    int32_t hlmtCount = 0;
    int32_t phmoCount = 0;
    int32_t collCount = 0;
    int32_t modeCount = 0;
    int32_t jmadCount = 0;

    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& hlmtEntry = m_Deps.State_Map.GetTag(i);
        if (hlmtEntry.TagGroupIndex < 0) continue;

        const uint32_t groupMagic =
            m_Deps.State_Map.GetGroupMagic(hlmtEntry.TagGroupIndex);
        if (groupMagic != MapMagics::k_HlmtMagic) continue;

        const std::string tagName = m_Deps.State_Map.GetTagName(i);
        if (tagName.empty()) continue;

        // Offset of the hlmt header in the file.
        const int64_t hlmtOffset = m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
        if (hlmtOffset < 0) continue;

        ++hlmtCount;

        // --- Phmo ---
        if (!m_Deps.State_MapPhmo.HasPhmo(tagName))
        {
            const Map_TagTableEntry phmoEntry =
                m_Deps.System_Header.ReadTagRef(file,
                    hlmtOffset + HlmtOffsets::k_PhysicsModel);

            const int64_t phmoOffset =
                m_Deps.System_Header.ResolveTagOffset(phmoEntry);

            if (phmoOffset >= 0)
            {
                auto phmo = std::make_unique<PhmoObject>(
                    reader.Read<PhmoObject>(file, phmoOffset, tagName));

                m_Deps.State_MapPhmo.AddPhmo(tagName, std::move(*phmo));
                ++phmoCount;
            }
        }

        // --- Coll ---
        if (!m_Deps.State_MapColl.HasColl(tagName))
        {
            const Map_TagTableEntry collEntry =
                m_Deps.System_Header.ReadTagRef(file,
                    hlmtOffset + HlmtOffsets::k_CollisionModel);

            const int64_t collOffset =
                m_Deps.System_Header.ResolveTagOffset(collEntry);

            if (collOffset >= 0)
            {
                auto coll = std::make_unique<CollObject>(
                    reader.Read<CollObject>(file, collOffset, tagName));

                m_Deps.State_MapColl.AddColl(tagName, std::move(*coll));
                collCount++;
            }
        }

        // --- Mode ---
        if (!m_Deps.State_MapMode.HasMode(tagName))
        {
            const Map_TagTableEntry modeEntry =
                m_Deps.System_Header.ReadTagRef(file,
                    hlmtOffset + HlmtOffsets::k_RenderModel);

            const int64_t modeOffset =
                m_Deps.System_Header.ResolveTagOffset(modeEntry);

            if (modeOffset >= 0)
            {
                auto mode = std::make_unique<ModeObject>(
                    reader.Read<ModeObject>(file, modeOffset, tagName));

                m_Deps.State_MapMode.AddMode(tagName, std::move(*mode));
                modeCount++;
            }
        }

        // --- Jmad ---
        if (!m_Deps.State_MapJmad.HasJmad(tagName))
        {
            const Map_TagTableEntry jmadEntry =
                m_Deps.System_Header.ReadTagRef(file,
                    hlmtOffset + HlmtOffsets::k_Animation);

            const int64_t jmadOffset = 
                m_Deps.System_Header.ResolveTagOffset(jmadEntry);

            if (jmadOffset >= 0)
            {
                auto jmad = std::make_unique<JmadObject>(
                    reader.Read<JmadObject>(file, jmadOffset, tagName));

                m_Deps.State_MapJmad.AddJmad(tagName, std::move(*jmad));
                jmadCount++;
            }
        }
    }

    m_Deps.System_Logs.Log("[MapTagGroupSystem] INFO: Raw built."
        " Hlmt processed: %d | Phmo loaded: %d | Coll loaded: %d |"
        " Mode loaded: %d | Jmad loaded: %d", 
        hlmtCount, phmoCount, collCount, modeCount, jmadCount);
}

void System_TagGroupReader::LoadDirectTags(FILE* file, TagGroupReader& reader)
{
    const int32_t tagCount = (int32_t)m_Deps.State_Map.GetTagsSize();
    
    int32_t vehiCount = 0;
    int32_t blocCount = 0;
    int32_t sbspCount = 0;
    int32_t weapCount = 0;
    int32_t projCount = 0;
    int32_t bipdCount = 0;
    int32_t eqipCount = 0;
    int32_t scenCount = 0;
    int32_t scnrCount = 0;
    int32_t ctrlCount = 0;
    int32_t machCount = 0;
    int32_t zoneCount = 0;
    int32_t sldtCount = 0;
    int32_t lbspCount = 0;
    int32_t playCount = 0;

    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& entry = m_Deps.State_Map.GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const uint32_t magic =
            m_Deps.State_Map.GetGroupMagic(entry.TagGroupIndex);

        std::string tagName = m_Deps.State_Map.GetTagName(i);

        if (tagName.empty())
        {
            if (magic == MapMagics::k_PlayMagic)
                tagName = "__play__";
            else if (magic == MapMagics::k_ZoneMagic)
                tagName = "__zone__";
            else
                continue;
        }

        // --- Zone ---
        if (magic == MapMagics::k_ZoneMagic &&
            !m_Deps.State_MapZone.HasZone(tagName))
        {
            m_Deps.System_Logs.Log("[MapTagGroup] Zone tagName: %s", tagName.c_str());

            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto zone = std::make_unique<ZoneObject>(
                reader.Read<ZoneObject>(file, offset, tagName));

            m_Deps.State_MapZone.AddZone(tagName, std::move(*zone));
            ++zoneCount;
        }

        // --- Play ---
        if (magic == MapMagics::k_PlayMagic &&
            !m_Deps.State_MapPlay.HasPlay(tagName))
        {
            std::string tagName = m_Deps.State_Map.GetTagName(i);
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto play = std::make_unique<PlayObject>(
                reader.Read<PlayObject>(file, offset, tagName));

            m_Deps.State_MapPlay.AddPlay(tagName, std::move(*play));
            ++playCount;
        }

        // --- Vehi ---
        if (magic == MapMagics::k_VehiMagic &&
            !m_Deps.State_MapVehi.HasVehi(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto vehi = std::make_unique<VehiObject>(
                reader.Read<VehiObject>(file, offset, tagName));

            m_Deps.State_MapVehi.AddVehi(tagName, std::move(*vehi));
            ++vehiCount;
        }

        // --- Bloc ---
        if (magic == MapMagics::k_BlocMagic &&
            !m_Deps.State_MapBloc.HasBloc(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bloc = std::make_unique<BlocObject>(
                reader.Read<BlocObject>(file, offset, tagName));

            m_Deps.State_MapBloc.AddBloc(tagName, std::move(*bloc));
            ++blocCount;
        }

        // --- Sbsp ---
        if (magic == MapMagics::k_SbspMagic &&
            !m_Deps.State_MapSbsp.HasSbsp(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto sbsp = std::make_unique<SbspObject>(
                reader.Read<SbspObject>(file, offset, tagName));

            m_Deps.State_MapSbsp.AddSbsp(tagName, std::move(*sbsp));
            ++sbspCount;
        }

        // --- Weap ---
        if (magic == MapMagics::k_WeapMagic &&
            !m_Deps.State_MapWeap.HasWeap(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto weap = std::make_unique<WeapObject>(
                reader.Read<WeapObject>(file, offset, tagName));

            m_Deps.State_MapWeap.AddWeap(tagName, std::move(*weap));
            ++weapCount;
        }

        // --- Proj ---
        if (magic == MapMagics::k_ProjMagic &&
            !m_Deps.State_MapProj.HasProj(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto proj = std::make_unique<ProjObject>(
                reader.Read<ProjObject>(file, offset, tagName));

            m_Deps.State_MapProj.AddProj(tagName, std::move(*proj));
            ++projCount;
        }

        // --- Bipd ---
        if (magic == MapMagics::k_BipdMagic &&
            !m_Deps.State_MapBipd.HasBipd(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bipd = std::make_unique<BipdObject>(
                reader.Read<BipdObject>(file, offset, tagName));

            m_Deps.State_MapBipd.AddBipd(tagName, std::move(*bipd));
            ++bipdCount;
        }

        // --- Eqip ---
        if (magic == MapMagics::k_EqipMagic &&
            !m_Deps.State_MapEqip.HasEqip(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bipd = std::make_unique<EqipObject>(
                reader.Read<EqipObject>(file, offset, tagName));

            m_Deps.State_MapEqip.AddEqip(tagName, std::move(*bipd));
            ++eqipCount;
        }

        // --- Scen ---
        if (magic == MapMagics::k_ScenMagic &&
            !m_Deps.State_MapScen.HasScen(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto scen = std::make_unique<ScenObject>(
                reader.Read<ScenObject>(file, offset, tagName));

            m_Deps.State_MapScen.AddScen(tagName, std::move(*scen));
            ++scenCount;
        }

        // --- Scnr ---
        if (magic == MapMagics::k_ScnrMagic &&
            !m_Deps.State_MapScnr.HasScnr(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto scnr = std::make_unique<ScnrObject>(
                reader.Read<ScnrObject>(file, offset, tagName));

            m_Deps.State_MapScnr.AddScnr(tagName, std::move(*scnr));
            ++scnrCount;
        }

        // --- Ctrl ---
        if (magic == MapMagics::k_CtrlMagic &&
            !m_Deps.State_MapCtrl.HasCtrl(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto ctrl = std::make_unique<CtrlObject>(
                reader.Read<CtrlObject>(file, offset, tagName));

            m_Deps.State_MapCtrl.AddCtrl(tagName, std::move(*ctrl));
            ++ctrlCount;
        }

        // --- Mach ---
        if (magic == MapMagics::k_MachMagic &&
            !m_Deps.State_MapMach.HasMach(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto mach = std::make_unique<MachObject>(
                reader.Read<MachObject>(file, offset, tagName));

            m_Deps.State_MapMach.AddMach(tagName, std::move(*mach));
            ++machCount;
        }

        // --- Sldt ---
        if (magic == MapMagics::k_SLdTMagic &&
            !m_Deps.State_MapSldt.HasSldt(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto sldt = std::make_unique<SldtObject>(
                reader.Read<SldtObject>(file, offset, tagName));

            m_Deps.State_MapSldt.AddSldt(tagName, std::move(*sldt));
            ++sldtCount;
        }

        // --- Lbsp ---
        if (magic == MapMagics::k_LbspMagic &&
            !m_Deps.State_MapLbsp.HasLbsp(tagName))
        {
            const int64_t offset =
                m_Deps.System_Header.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto lbsp = std::make_unique<LbspObject>(
                reader.Read<LbspObject>(file, offset, tagName));

            m_Deps.State_MapLbsp.AddLbsp(tagName, std::move(*lbsp));
            ++lbspCount;
        }
    }

    m_Deps.System_Logs.Log("[MapTagGroupSystem] INFO: Raw built."
        " Vehi loaded: %d | Bloc loaded: %d | Sbsp loaded: %d |"
        " Weap loaded: %d | Proj loaded: %d | Bipd loaded: %d |"
        " Eqip loaded: %d | Scen loaded: %d | Sncr loaded: %d |"
        " Ctrl loaded: %d | Mach loaded: %d | Zone loaded: %d |"
        " Play loaded: %d | Sldt loaded: %d | Lbsp loaded: %d", 
        vehiCount, blocCount, sbspCount, weapCount, projCount, bipdCount, 
        eqipCount, scenCount, scnrCount, ctrlCount, machCount, zoneCount,
        playCount, sldtCount, lbspCount);
}