#include "pch.h"

// Header.
#include "System_MapTagGroup.h"

// Types.
#include "Core/Types/Domain/Map/MapMagics.h"

// --- States ---

// Map.
#include "Core/States/Domain/Map/State_Map.h"
#include "Core/States/Domain/Map/Phmo/State_MapPhmo.h"
#include "Core/States/Domain/Map/Coll/State_MapColl.h"
#include "Core/States/Domain/Map/Mode/State_MapMode.h"
#include "Core/States/Domain/Map/Vehi/State_MapVehi.h"
#include "Core/States/Domain/Map/Bloc/State_MapBloc.h"
#include "Core/States/Domain/Map/Sbsp/State_MapSbsp.h"
#include "Core/States/Domain/Map/Weap/State_MapWeap.h"
#include "Core/States/Domain/Map/Proj/State_MapProj.h"
#include "Core/States/Domain/Map/Bipd/State_MapBipd.h"
#include "Core/States/Domain/Map/Eqip/State_MapEqip.h"
#include "Core/States/Domain/Map/Scen/State_MapScen.h"
#include "Core/States/Domain/Map/Scnr/State_MapScnr.h"
#include "Core/States/Domain/Map/Jmad/State_MapJmad.h"
#include "Core/States/Domain/Map/Ctrl/State_MapCtrl.h"
#include "Core/States/Domain/Map/Mach/State_MapMach.h"

// --- Systems ---

// Map.
#include "Core/Systems/Domain/Map/System_Map.h"
#include "Core/Systems/Domain/Map/TagGroupReader.h"

// Debug.
#include "Core/Systems/Interface/Debug/System_Debug.h"

#include "Generated/Hlmt/HlmtOffsets.h" 

void System_MapTagGroup::LoadForMap()
{
    FILE* file = m_Dependencies.System_Map.OpenMapFile();
    if (!file)
    {
        m_Dependencies.System_Debug.Log("[MapTagGroupSystem] ERROR:"
            " Could not open map file.");
        return;
    }

    TagGroupReader reader(m_Dependencies.System_Map);

    this->LoadHlmtFamily(file, reader);
    this->LoadDirectTags(file, reader);

    fclose(file);
    m_Dependencies.System_Debug.Log("[MapTagGroupSystem] INFO:"
        " Load completed.");
}

void System_MapTagGroup::Cleanup()
{
    // Hlmt-derived.
    m_Dependencies.State_MapPhmo.Cleanup();
    m_Dependencies.State_MapColl.Cleanup();
    m_Dependencies.State_MapMode.Cleanup();
    m_Dependencies.State_MapJmad.Cleanup();

    // Direct TagTable entry.
    m_Dependencies.State_MapVehi.Cleanup();
    m_Dependencies.State_MapBloc.Cleanup();
    m_Dependencies.State_MapSbsp.Cleanup();
    m_Dependencies.State_MapWeap.Cleanup();
    m_Dependencies.State_MapProj.Cleanup();
    m_Dependencies.State_MapBipd.Cleanup();
    m_Dependencies.State_MapEqip.Cleanup();
    m_Dependencies.State_MapScen.Cleanup();
    m_Dependencies.State_MapScnr.Cleanup();
    m_Dependencies.State_MapCtrl.Cleanup();

    m_Dependencies.System_Debug.Log("[MapTagGroupSystem] INFO:"
        " Cleanup completed.");
}

void System_MapTagGroup::LoadHlmtFamily(FILE* file, TagGroupReader& reader)
{
    const int32_t tagCount = (int32_t)m_Dependencies.State_Map.GetTagsSize();
    
    int32_t hlmtCount = 0;
    int32_t phmoCount = 0;
    int32_t collCount = 0;
    int32_t modeCount = 0;
    int32_t jmadCount = 0;

    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& hlmtEntry = m_Dependencies.State_Map.GetTag(i);
        if (hlmtEntry.TagGroupIndex < 0) continue;

        const uint32_t groupMagic =
            m_Dependencies.State_Map.GetGroupMagic(hlmtEntry.TagGroupIndex);
        if (groupMagic != MapMagics::k_HlmtMagic) continue;

        const std::string tagName = m_Dependencies.State_Map.GetTagName(i);
        if (tagName.empty()) continue;

        // Offset of the hlmt header in the file.
        const int64_t hlmtOffset = m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
        if (hlmtOffset < 0) continue;

        ++hlmtCount;

        // --- Phmo ---
        if (!m_Dependencies.State_MapPhmo.HasPhmo(tagName))
        {
            const Map_TagTableEntry phmoEntry =
                m_Dependencies.System_Map.ReadTagRef(file,
                    hlmtOffset + HlmtOffsets::k_PhysicsModel);

            const int64_t phmoOffset =
                m_Dependencies.System_Map.ResolveTagOffset(phmoEntry);

            if (phmoOffset >= 0)
            {
                auto phmo = std::make_unique<PhmoObject>(
                    reader.Read<PhmoObject>(file, phmoOffset, tagName));

                m_Dependencies.State_MapPhmo.AddPhmo(tagName, std::move(*phmo));
                ++phmoCount;
            }
        }

        // --- Coll ---
        if (!m_Dependencies.State_MapColl.HasColl(tagName))
        {
            const Map_TagTableEntry collEntry =
                m_Dependencies.System_Map.ReadTagRef(file,
                    hlmtOffset + HlmtOffsets::k_CollisionModel);

            const int64_t collOffset =
                m_Dependencies.System_Map.ResolveTagOffset(collEntry);

            if (collOffset >= 0)
            {
                auto coll = std::make_unique<CollObject>(
                    reader.Read<CollObject>(file, collOffset, tagName));

                m_Dependencies.State_MapColl.AddColl(tagName, std::move(*coll));
                collCount++;
            }
        }

        // --- Mode ---
        if (!m_Dependencies.State_MapMode.HasMode(tagName))
        {
            const Map_TagTableEntry modeEntry =
                m_Dependencies.System_Map.ReadTagRef(file,
                    hlmtOffset + HlmtOffsets::k_RenderModel);

            const int64_t modeOffset =
                m_Dependencies.System_Map.ResolveTagOffset(modeEntry);

            if (modeOffset >= 0)
            {
                auto mode = std::make_unique<ModeObject>(
                    reader.Read<ModeObject>(file, modeOffset, tagName));

                m_Dependencies.State_MapMode.AddMode(tagName, std::move(*mode));
                modeCount++;
            }
        }

        // --- Jmad ---
        if (!m_Dependencies.State_MapJmad.HasJmad(tagName))
        {
            const Map_TagTableEntry jmadEntry =
                m_Dependencies.System_Map.ReadTagRef(file,
                    hlmtOffset + HlmtOffsets::k_Animation);

            const int64_t jmadOffset = 
                m_Dependencies.System_Map.ResolveTagOffset(jmadEntry);

            if (jmadOffset >= 0)
            {
                auto jmad = std::make_unique<JmadObject>(
                    reader.Read<JmadObject>(file, jmadOffset, tagName));

                m_Dependencies.State_MapJmad.AddJmad(tagName, std::move(*jmad));
                jmadCount++;
            }
        }
    }

    m_Dependencies.System_Debug.Log("[MapTagGroupSystem] INFO: Raw built."
        " Hlmt processed: %d | Phmo loaded: %d | Coll loaded: %d |"
        " Mode loaded: %d | Jmad loaded: %d", 
        hlmtCount, phmoCount, collCount, modeCount, jmadCount);
}

void System_MapTagGroup::LoadDirectTags(FILE* file, TagGroupReader& reader)
{
    const int32_t tagCount = (int32_t)m_Dependencies.State_Map.GetTagsSize();
    
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

    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& entry = m_Dependencies.State_Map.GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const uint32_t magic =
            m_Dependencies.State_Map.GetGroupMagic(entry.TagGroupIndex);
        const std::string tagName = m_Dependencies.State_Map.GetTagName(i);
        if (tagName.empty()) continue;

        // --- Vehi ---
        if (magic == MapMagics::k_VehiMagic &&
            !m_Dependencies.State_MapVehi.HasVehi(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto vehi = std::make_unique<VehiObject>(
                reader.Read<VehiObject>(file, offset, tagName));

            m_Dependencies.State_MapVehi.AddVehi(tagName, std::move(*vehi));
            ++vehiCount;
        }

        // --- Bloc ---
        if (magic == MapMagics::k_BlocMagic &&
            !m_Dependencies.State_MapBloc.HasBloc(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bloc = std::make_unique<BlocObject>(
                reader.Read<BlocObject>(file, offset, tagName));

            m_Dependencies.State_MapBloc.AddBloc(tagName, std::move(*bloc));
            ++blocCount;
        }

        // --- Sbsp ---
        if (magic == MapMagics::k_SbspMagic &&
            !m_Dependencies.State_MapSbsp.HasSbsp(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto sbsp = std::make_unique<SbspObject>(
                reader.Read<SbspObject>(file, offset, tagName));

            m_Dependencies.State_MapSbsp.AddSbsp(tagName, std::move(*sbsp));
            ++sbspCount;
        }

        // --- Weap ---
        if (magic == MapMagics::k_WeapMagic &&
            !m_Dependencies.State_MapWeap.HasWeap(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto weap = std::make_unique<WeapObject>(
                reader.Read<WeapObject>(file, offset, tagName));

            m_Dependencies.State_MapWeap.AddWeap(tagName, std::move(*weap));
            ++weapCount;
        }

        // --- Proj ---
        if (magic == MapMagics::k_ProjMagic &&
            !m_Dependencies.State_MapProj.HasProj(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto proj = std::make_unique<ProjObject>(
                reader.Read<ProjObject>(file, offset, tagName));

            m_Dependencies.State_MapProj.AddProj(tagName, std::move(*proj));
            ++projCount;
        }

        // --- Bipd ---
        if (magic == MapMagics::k_BipdMagic &&
            !m_Dependencies.State_MapBipd.HasBipd(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bipd = std::make_unique<BipdObject>(
                reader.Read<BipdObject>(file, offset, tagName));

            m_Dependencies.State_MapBipd.AddBipd(tagName, std::move(*bipd));
            ++bipdCount;
        }

        // --- Eqip ---
        if (magic == MapMagics::k_EqipMagic &&
            !m_Dependencies.State_MapEqip.HasEqip(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bipd = std::make_unique<EqipObject>(
                reader.Read<EqipObject>(file, offset, tagName));

            m_Dependencies.State_MapEqip.AddEqip(tagName, std::move(*bipd));
            ++eqipCount;
        }

        // --- Scen ---
        if (magic == MapMagics::k_ScenMagic &&
            !m_Dependencies.State_MapScen.HasScen(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto scen = std::make_unique<ScenObject>(
                reader.Read<ScenObject>(file, offset, tagName));

            m_Dependencies.State_MapScen.AddScen(tagName, std::move(*scen));
            ++scenCount;
        }

        // --- Scnr ---
        if (magic == MapMagics::k_ScnrMagic &&
            !m_Dependencies.State_MapScnr.HasScnr(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto scnr = std::make_unique<ScnrObject>(
                reader.Read<ScnrObject>(file, offset, tagName));

            m_Dependencies.State_MapScnr.AddScnr(tagName, std::move(*scnr));
            ++scnrCount;
        }

        // --- Ctrl ---
        if (magic == MapMagics::k_CtrlMagic &&
            !m_Dependencies.State_MapCtrl.HasCtrl(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto ctrl = std::make_unique<CtrlObject>(
                reader.Read<CtrlObject>(file, offset, tagName));

            m_Dependencies.State_MapCtrl.AddCtrl(tagName, std::move(*ctrl));
            ++ctrlCount;
        }

        // --- Mach ---
        if (magic == MapMagics::k_MachMagic &&
            !m_Dependencies.State_MapMach.HasMach(tagName))
        {
            const int64_t offset =
                m_Dependencies.System_Map.GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto mach = std::make_unique<MachObject>(
                reader.Read<MachObject>(file, offset, tagName));

            m_Dependencies.State_MapMach.AddMach(tagName, std::move(*mach));
            ++machCount;
        }
    }

    m_Dependencies.System_Debug.Log("[MapTagGroupSystem] INFO: Raw built."
        " Vehi loaded: %d | Bloc loaded: %d | Sbsp loaded: %d |"
        " Weap loaded: %d | Proj loaded: %d | Bipd loaded: %d |"
        " Eqip loaded: %d | Scen loaded: %d | Sncr loaded: %d |"
        " Ctrl loaded: %d | Mach loaded: %d", 
        vehiCount, blocCount, sbspCount, weapCount, projCount, bipdCount, 
        eqipCount, scenCount, scnrCount, ctrlCount, machCount);
}