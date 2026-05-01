#include "pch.h"
#include "Core/Common/Map/MapMagics.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
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
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Map/System_Map.h"
#include "Core/Systems/Domain/Map/System_MapTagGroup.h"
#include "Core/Systems/Domain/Map/TagGroupReader.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "Generated/Hlmt/HlmtOffsets.h" 

void System_MapTagGroup::LoadForMap()
{
    FILE* file = g_pSystem->Domain->Map->OpenMapFile();
    if (!file)
    {
        g_pSystem->Debug->Log("[MapTagGroupSystem] ERROR:"
            " Could not open map file.");
        return;
    }

    TagGroupReader reader(*g_pSystem->Domain->Map);

    this->LoadHlmtFamily(file, reader);
    this->LoadDirectTags(file, reader);

    fclose(file);
    g_pSystem->Debug->Log("[MapTagGroupSystem] INFO: Load completed.");
}

void System_MapTagGroup::Cleanup()
{
    // Hlmt-derived.
    g_pState->Domain->MapPhmo->Cleanup();
    g_pState->Domain->MapColl->Cleanup();
    g_pState->Domain->MapMode->Cleanup();
    g_pState->Domain->MapJmad->Cleanup();

    // Direct TagTable entry.
    g_pState->Domain->MapVehi->Cleanup();
    g_pState->Domain->MapBloc->Cleanup();
    g_pState->Domain->MapSbsp->Cleanup();
    g_pState->Domain->MapWeap->Cleanup();
    g_pState->Domain->MapProj->Cleanup();
    g_pState->Domain->MapBipd->Cleanup();
    g_pState->Domain->MapEqip->Cleanup();
    g_pState->Domain->MapScen->Cleanup();
    g_pState->Domain->MapScnr->Cleanup();
    g_pState->Domain->MapCtrl->Cleanup();

    g_pSystem->Debug->Log("[MapTagGroupSystem] INFO: Cleanup completed.");
}

void System_MapTagGroup::LoadHlmtFamily(FILE* file, TagGroupReader& reader)
{
    const int32_t tagCount = (int32_t)g_pState->Domain->Map->GetTagsSize();
    
    int32_t hlmtCount = 0;
    int32_t phmoCount = 0;
    int32_t collCount = 0;
    int32_t modeCount = 0;
    int32_t jmadCount = 0;

    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& hlmtEntry = g_pState->Domain->Map->GetTag(i);
        if (hlmtEntry.TagGroupIndex < 0) continue;

        const uint32_t groupMagic =
            g_pState->Domain->Map->GetGroupMagic(hlmtEntry.TagGroupIndex);
        if (groupMagic != MapMagics::k_HlmtMagic) continue;

        const std::string tagName = g_pState->Domain->Map->GetTagName(i);
        if (tagName.empty()) continue;

        // Offset of the hlmt header in the file.
        const int64_t hlmtOffset =
            g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
        if (hlmtOffset < 0) continue;

        ++hlmtCount;

        // --- Phmo ---
        if (!g_pState->Domain->MapPhmo->HasPhmo(tagName))
        {
            const Map_TagTableEntry phmoEntry =
                g_pSystem->Domain->Map->ReadTagRef(
                    file, hlmtOffset + HlmtOffsets::k_PhysicsModel);

            const int64_t phmoOffset =
                g_pSystem->Domain->Map->ResolveTagOffset(phmoEntry);

            if (phmoOffset >= 0)
            {
                auto phmo = std::make_unique<PhmoObject>(
                    reader.Read<PhmoObject>(file, phmoOffset, tagName));

                g_pState->Domain->MapPhmo->AddPhmo(tagName, std::move(*phmo));
                ++phmoCount;
            }
        }

        // --- Coll ---
        if (!g_pState->Domain->MapColl->HasColl(tagName))
        {
            const Map_TagTableEntry collEntry =
                g_pSystem->Domain->Map->ReadTagRef(
                    file, hlmtOffset + HlmtOffsets::k_CollisionModel);

            const int64_t collOffset =
                g_pSystem->Domain->Map->ResolveTagOffset(collEntry);

            if (collOffset >= 0)
            {
                auto coll = std::make_unique<CollObject>(
                    reader.Read<CollObject>(file, collOffset, tagName));

                g_pState->Domain->MapColl->AddColl(tagName, std::move(*coll));
                collCount++;
            }
        }

        // --- Mode ---
        if (!g_pState->Domain->MapMode->HasMode(tagName))
        {
            const Map_TagTableEntry modeEntry =
                g_pSystem->Domain->Map->ReadTagRef(
                    file, hlmtOffset + HlmtOffsets::k_RenderModel);

            const int64_t modeOffset =
                g_pSystem->Domain->Map->ResolveTagOffset(modeEntry);

            if (modeOffset >= 0)
            {
                auto mode = std::make_unique<ModeObject>(
                    reader.Read<ModeObject>(file, modeOffset, tagName));

                g_pState->Domain->MapMode->AddMode(tagName, std::move(*mode));
                modeCount++;
            }
        }

        // --- Jmad ---
        if (!g_pState->Domain->MapJmad->HasJmad(tagName))
        {
            const Map_TagTableEntry jmadEntry =
                g_pSystem->Domain->Map->ReadTagRef(
                    file, hlmtOffset + HlmtOffsets::k_Animation);

            const int64_t jmadOffset =
                g_pSystem->Domain->Map->ResolveTagOffset(jmadEntry);

            if (jmadOffset >= 0)
            {
                auto jmad = std::make_unique<JmadObject>(
                    reader.Read<JmadObject>(file, jmadOffset, tagName));

                g_pState->Domain->MapJmad->AddJmad(tagName, std::move(*jmad));
                jmadCount++;
            }
        }
    }

    g_pSystem->Debug->Log("[MapTagGroupSystem] INFO: Raw built."
        " Hlmt processed: %d | Phmo loaded: %d | Coll loaded: %d |"
        " Mode loaded: %d | Jmad loaded: %d", 
        hlmtCount, phmoCount, collCount, modeCount, jmadCount);
}

void System_MapTagGroup::LoadDirectTags(FILE* file, TagGroupReader& reader)
{
    const int32_t tagCount = (int32_t)g_pState->Domain->Map->GetTagsSize();
    
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
        const Map_TagTableEntry& entry = g_pState->Domain->Map->GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const uint32_t magic =
            g_pState->Domain->Map->GetGroupMagic(entry.TagGroupIndex);
        const std::string tagName = g_pState->Domain->Map->GetTagName(i);
        if (tagName.empty()) continue;

        // --- Vehi ---
        if (magic == MapMagics::k_VehiMagic &&
            !g_pState->Domain->MapVehi->HasVehi(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto vehi = std::make_unique<VehiObject>(
                reader.Read<VehiObject>(file, offset, tagName));

            g_pState->Domain->MapVehi->AddVehi(tagName, std::move(*vehi));
            ++vehiCount;
        }

        // --- Bloc ---
        if (magic == MapMagics::k_BlocMagic &&
            !g_pState->Domain->MapBloc->HasBloc(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bloc = std::make_unique<BlocObject>(
                reader.Read<BlocObject>(file, offset, tagName));

            g_pState->Domain->MapBloc->AddBloc(tagName, std::move(*bloc));
            ++blocCount;
        }

        // --- Sbsp ---
        if (magic == MapMagics::k_SbspMagic &&
            !g_pState->Domain->MapSbsp->HasSbsp(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto sbsp = std::make_unique<SbspObject>(
                reader.Read<SbspObject>(file, offset, tagName));

            g_pState->Domain->MapSbsp->AddSbsp(tagName, std::move(*sbsp));
            ++sbspCount;
        }

        // --- Weap ---
        if (magic == MapMagics::k_WeapMagic &&
            !g_pState->Domain->MapWeap->HasWeap(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto weap = std::make_unique<WeapObject>(
                reader.Read<WeapObject>(file, offset, tagName));

            g_pState->Domain->MapWeap->AddWeap(tagName, std::move(*weap));
            ++weapCount;
        }

        // --- Proj ---
        if (magic == MapMagics::k_ProjMagic &&
            !g_pState->Domain->MapProj->HasProj(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto proj = std::make_unique<ProjObject>(
                reader.Read<ProjObject>(file, offset, tagName));

            g_pState->Domain->MapProj->AddProj(tagName, std::move(*proj));
            ++projCount;
        }

        // --- Bipd ---
        if (magic == MapMagics::k_BipdMagic &&
            !g_pState->Domain->MapBipd->HasBipd(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bipd = std::make_unique<BipdObject>(
                reader.Read<BipdObject>(file, offset, tagName));

            g_pState->Domain->MapBipd->AddBipd(tagName, std::move(*bipd));
            ++bipdCount;
        }

        // --- Eqip ---
        if (magic == MapMagics::k_EqipMagic &&
            !g_pState->Domain->MapEqip->HasEqip(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto bipd = std::make_unique<EqipObject>(
                reader.Read<EqipObject>(file, offset, tagName));

            g_pState->Domain->MapEqip->AddEqip(tagName, std::move(*bipd));
            ++eqipCount;
        }

        // --- Scen ---
        if (magic == MapMagics::k_ScenMagic &&
            !g_pState->Domain->MapScen->HasScen(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto scen = std::make_unique<ScenObject>(
                reader.Read<ScenObject>(file, offset, tagName));

            g_pState->Domain->MapScen->AddScen(tagName, std::move(*scen));
            ++scenCount;
        }

        // --- Scnr ---
        if (magic == MapMagics::k_ScnrMagic &&
            !g_pState->Domain->MapScnr->HasScnr(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto scnr = std::make_unique<ScnrObject>(
                reader.Read<ScnrObject>(file, offset, tagName));

            g_pState->Domain->MapScnr->AddScnr(tagName, std::move(*scnr));
            ++scnrCount;
        }

        // --- Ctrl ---
        if (magic == MapMagics::k_CtrlMagic &&
            !g_pState->Domain->MapCtrl->HasCtrl(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto ctrl = std::make_unique<CtrlObject>(
                reader.Read<CtrlObject>(file, offset, tagName));

            g_pState->Domain->MapCtrl->AddCtrl(tagName, std::move(*ctrl));
            ++ctrlCount;
        }

        // --- Mach ---
        if (magic == MapMagics::k_MachMagic &&
            !g_pState->Domain->MapMach->HasMach(tagName))
        {
            const int64_t offset =
                g_pSystem->Domain->Map->GetTagMetaOffsetByIndex(i);
            if (offset < 0) continue;

            auto mach = std::make_unique<MachObject>(
                reader.Read<MachObject>(file, offset, tagName));

            g_pState->Domain->MapMach->AddMach(tagName, std::move(*mach));
            ++machCount;
        }
    }

    g_pSystem->Debug->Log("[MapTagGroupSystem] INFO: Raw built."
        " Vehi loaded: %d | Bloc loaded: %d | Sbsp loaded: %d |"
        " Weap loaded: %d | Proj loaded: %d | Bipd loaded: %d |"
        " Eqip loaded: %d | Scen loaded: %d | Sncr loaded: %d |"
        " Ctrl loaded: %d | Mach loaded: %d", 
        vehiCount, blocCount, sbspCount, weapCount, projCount, bipdCount, 
        eqipCount, scenCount, scnrCount, ctrlCount, machCount);
}