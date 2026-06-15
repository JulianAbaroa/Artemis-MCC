#include "pch.h"

#include "System_TagGroupReader.h"

#include "Core/Types/Sources/MapReader/MapMagics.h"

#include "Core/States/Sources/MapReader/State_MapReader.h"
#include "Core/States/Sources/MapReader/Tags/State_MapBipd.h"
#include "Core/States/Sources/MapReader/Tags/State_MapBloc.h"
#include "Core/States/Sources/MapReader/Tags/State_MapColl.h"
#include "Core/States/Sources/MapReader/Tags/State_MapCtrl.h"
#include "Core/States/Sources/MapReader/Tags/State_MapEqip.h"
#include "Core/States/Sources/MapReader/Tags/State_MapHlmt.h"
#include "Core/States/Sources/MapReader/Tags/State_MapLbsp.h"
#include "Core/States/Sources/MapReader/Tags/State_MapMach.h"
#include "Core/States/Sources/MapReader/Tags/State_MapMode.h"
#include "Core/States/Sources/MapReader/Tags/State_MapPhmo.h"
#include "Core/States/Sources/MapReader/Tags/State_MapPlay.h"
#include "Core/States/Sources/MapReader/Tags/State_MapProj.h"
#include "Core/States/Sources/MapReader/Tags/State_MapSbsp.h"
#include "Core/States/Sources/MapReader/Tags/State_MapScen.h"
#include "Core/States/Sources/MapReader/Tags/State_MapScnr.h"
#include "Core/States/Sources/MapReader/Tags/State_MapSldt.h"
#include "Core/States/Sources/MapReader/Tags/State_MapVehi.h"
#include "Core/States/Sources/MapReader/Tags/State_MapWeap.h"
#include "Core/States/Sources/MapReader/Tags/State_MapZone.h"

#include "TagGroupReader.h"
#include "../System_MapReader.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

#include "Generated/Bipd/BipdObject.h"
#include "Generated/Bipd/BipdDescriptor.h"
#include "Generated/Bloc/BlocObject.h"
#include "Generated/Bloc/BlocDescriptor.h"
#include "Generated/Coll/CollObject.h"
#include "Generated/Coll/CollDescriptor.h"
#include "Generated/Ctrl/CtrlObject.h"
#include "Generated/Ctrl/CtrlDescriptor.h"
#include "Generated/Eqip/EqipObject.h"
#include "Generated/Eqip/EqipDescriptor.h"
#include "Generated/Hlmt/HlmtObject.h"
#include "Generated/Hlmt/HlmtDescriptor.h"
#include "Generated/Lbsp/LbspObject.h"
#include "Generated/Lbsp/LbspDescriptor.h"
#include "Generated/Mach/MachObject.h"
#include "Generated/Mach/MachDescriptor.h"
#include "Generated/Mode/ModeObject.h"
#include "Generated/Mode/ModeDescriptor.h"
#include "Generated/Phmo/PhmoObject.h" 
#include "Generated/Phmo/PhmoDescriptor.h"
#include "Generated/Play/PlayObject.h"
#include "Generated/Play/PlayDescriptor.h"
#include "Generated/Proj/ProjObject.h"
#include "Generated/Proj/ProjDescriptor.h"
#include "Generated/Sbsp/SbspObject.h"
#include "Generated/Sbsp/SbspDescriptor.h"
#include "Generated/Scen/ScenObject.h"
#include "Generated/Scen/ScenDescriptor.h"
#include "Generated/Scnr/ScnrObject.h"
#include "Generated/Scnr/ScnrDescriptor.h"
#include "Generated/Sldt/SldtObject.h"
#include "Generated/Sldt/SldtDescriptor.h"
#include "Generated/Vehi/VehiObject.h"
#include "Generated/Vehi/VehiDescriptor.h"
#include "Generated/Weap/WeapObject.h"
#include "Generated/Weap/WeapDescriptor.h"
#include "Generated/Zone/ZoneObject.h"
#include "Generated/Zone/ZoneDescriptor.h"

void System_TagGroupReader::LoadForMap()
{
    const std::string& filePath = 
        m_Deps.State_MapReader.GetMapFilePath();

    FILE* file = m_Deps.System_MapReader.OpenMapFile(filePath);
    if (!file)
    {
        m_Deps.System_Logs.Log("[TagGroupReader] ERROR:"
            " Could not open map file.");
        return;
    }

    TagGroupReader reader(m_Deps.System_MapReader);

    this->LoadTags(file, reader);

    fclose(file);

    m_Deps.System_Logs.Log("[TagGroupReader] INFO:"
        " Load completed.");
}

void System_TagGroupReader::LoadTags(FILE* file, TagGroupReader& reader)
{
    const int32_t tagCount = (int32_t)m_Deps.
        State_MapReader.GetTagsSize();

    int32_t bipd{}, bloc{}, coll{}, ctrl{}, eqip{}, hlmt{}, 
        lbsp{}, mach{}, mode{}, phmo{}, play{}, proj{}, sbsp{}, 
        scen{},  scnr{}, sldt{}, vehi{}, weap{}, zone{};

    for (int32_t i = 0; i < tagCount; ++i)
    {
        const Map_TagTableEntry& entry = m_Deps.State_MapReader.GetTag(i);
        if (entry.TagGroupIndex < 0) continue;

        const uint32_t magic =
            m_Deps.State_MapReader.GetGroupMagic(entry.TagGroupIndex);

        std::string tagName = m_Deps.State_MapReader.GetTagName(i);

        if (tagName.empty())
        {
            if (magic == MapMagics::k_PlayMagic) 
            {
                tagName = "__play__";
            }
            else if (magic == MapMagics::k_ZoneMagic)
            {
                tagName = "__zone__";
            }
            else continue;
        }

        // --- Bipd ---
        if (magic == MapMagics::k_BipdMagic &&
            !m_Deps.State_MapBipd.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<BipdObject>(
                    reader.Read<BipdObject>(file, offset, tagName));

                m_Deps.State_MapBipd.Add(tagName, std::move(*object));
                ++bipd;
            }
        }

        // --- Bloc ---
        if (magic == MapMagics::k_BlocMagic &&
            !m_Deps.State_MapBloc.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<BlocObject>(
                    reader.Read<BlocObject>(file, offset, tagName));

                m_Deps.State_MapBloc.Add(tagName, std::move(*object));
                ++bloc;
            }
        }

        // --- Coll ---
        if (magic == MapMagics::k_CollMagic &&
            !m_Deps.State_MapColl.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<CollObject>(
                    reader.Read<CollObject>(file, offset, tagName));

                m_Deps.State_MapColl.Add(tagName, std::move(*object));
                coll++;
            }
        }

        // --- Ctrl ---
        if (magic == MapMagics::k_CtrlMagic &&
            !m_Deps.State_MapCtrl.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<CtrlObject>(
                    reader.Read<CtrlObject>(file, offset, tagName));

                m_Deps.State_MapCtrl.Add(tagName, std::move(*object));
                ++ctrl;
            }
        }

        // --- Eqip ---
        if (magic == MapMagics::k_EqipMagic &&
            !m_Deps.State_MapEqip.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<EqipObject>(
                    reader.Read<EqipObject>(file, offset, tagName));

                m_Deps.State_MapEqip.Add(tagName, std::move(*object));
                ++eqip;
            }
        }

        // --- Hlmt ---
        if (magic == MapMagics::k_HlmtMagic &&
            !m_Deps.State_MapHlmt.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<HlmtObject>(
                    reader.Read<HlmtObject>(file, offset, tagName));

                m_Deps.State_MapHlmt.Add(tagName, std::move(*object));
                ++hlmt;
            }
        }

        // --- Lbsp ---
        if (magic == MapMagics::k_LbspMagic &&
            !m_Deps.State_MapLbsp.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<LbspObject>(
                    reader.Read<LbspObject>(file, offset, tagName));

                m_Deps.State_MapLbsp.Add(tagName, std::move(*object));
                ++lbsp;
            }
        }

        // --- Mach ---
        if (magic == MapMagics::k_MachMagic &&
            !m_Deps.State_MapMach.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<MachObject>(
                    reader.Read<MachObject>(file, offset, tagName));

                m_Deps.State_MapMach.Add(tagName, std::move(*object));
                ++mach;
            }
        }

        // --- Mode ---
        if (magic == MapMagics::k_ModeMagic &&
            !m_Deps.State_MapMode.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<ModeObject>(
                    reader.Read<ModeObject>(file, offset, tagName));

                m_Deps.State_MapMode.Add(tagName, std::move(*object));
                mode++;
            }
        }

        // --- Phmo ---
        if (magic == MapMagics::k_PhmoMagic &&
            !m_Deps.State_MapPhmo.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<PhmoObject>(
                    reader.Read<PhmoObject>(file, offset, tagName));

                m_Deps.State_MapPhmo.Add(tagName, std::move(*object));
                ++phmo;
            }
        }

        // --- Play ---
        if (magic == MapMagics::k_PlayMagic &&
            !m_Deps.State_MapPlay.Has(tagName))
        {
            std::string tagName = m_Deps.State_MapReader.GetTagName(i);
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<PlayObject>(
                    reader.Read<PlayObject>(file, offset, tagName));

                m_Deps.State_MapPlay.Add(tagName, std::move(*object));
                ++play;
            }
        }

        // --- Proj ---
        if (magic == MapMagics::k_ProjMagic &&
            !m_Deps.State_MapProj.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<ProjObject>(
                    reader.Read<ProjObject>(file, offset, tagName));

                m_Deps.State_MapProj.Add(tagName, std::move(*object));
                ++proj;
            }
        }

        // --- Sbsp ---
        if (magic == MapMagics::k_SbspMagic &&
            !m_Deps.State_MapSbsp.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<SbspObject>(
                    reader.Read<SbspObject>(file, offset, tagName));

                m_Deps.State_MapSbsp.Add(tagName, std::move(*object));
                ++sbsp;
            }
        }

        // --- Scen ---
        if (magic == MapMagics::k_ScenMagic &&
            !m_Deps.State_MapScen.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<ScenObject>(
                    reader.Read<ScenObject>(file, offset, tagName));

                m_Deps.State_MapScen.Add(tagName, std::move(*object));
                ++scen;
            }
        }

        // --- Scnr ---
        if (magic == MapMagics::k_ScnrMagic &&
            !m_Deps.State_MapScnr.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<ScnrObject>(
                    reader.Read<ScnrObject>(file, offset, tagName));

                m_Deps.State_MapScnr.Add(tagName, std::move(*object));
                ++scnr;
            }

        }

        // --- Sldt ---
        if (magic == MapMagics::k_SLdTMagic &&
            !m_Deps.State_MapSldt.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<SldtObject>(
                    reader.Read<SldtObject>(file, offset, tagName));

                m_Deps.State_MapSldt.Add(tagName, std::move(*object));
                ++sldt;
            }
        }

        // --- Vehi ---
        if (magic == MapMagics::k_VehiMagic &&
            !m_Deps.State_MapVehi.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<VehiObject>(
                    reader.Read<VehiObject>(file, offset, tagName));

                m_Deps.State_MapVehi.Add(tagName, std::move(*object));
                ++vehi;
            }
        }

        // --- Weap ---
        if (magic == MapMagics::k_WeapMagic &&
            !m_Deps.State_MapWeap.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<WeapObject>(
                    reader.Read<WeapObject>(file, offset, tagName));

                m_Deps.State_MapWeap.Add(tagName, std::move(*object));
                ++weap;
            }
        }

        // --- Zone ---
        if (magic == MapMagics::k_ZoneMagic &&
            !m_Deps.State_MapZone.Has(tagName))
        {
            const int64_t offset =
                m_Deps.System_MapReader.GetTagOffset(i);

            if (offset >= 0)
            {
                auto object = std::make_unique<ZoneObject>(
                    reader.Read<ZoneObject>(file, offset, tagName));

                m_Deps.State_MapZone.Add(tagName, std::move(*object));
                ++zone;
            }
        }
    }

    m_Deps.State_MapBipd.Freeze();
    m_Deps.State_MapBloc.Freeze();
    m_Deps.State_MapColl.Freeze();
    m_Deps.State_MapCtrl.Freeze();
    m_Deps.State_MapEqip.Freeze();
    m_Deps.State_MapHlmt.Freeze();
    m_Deps.State_MapLbsp.Freeze();
    m_Deps.State_MapMach.Freeze();
    m_Deps.State_MapPlay.Freeze();
    m_Deps.State_MapMode.Freeze();
    m_Deps.State_MapPhmo.Freeze();
    m_Deps.State_MapProj.Freeze();
    m_Deps.State_MapSbsp.Freeze();
    m_Deps.State_MapScen.Freeze();
    m_Deps.State_MapScnr.Freeze();
    m_Deps.State_MapSldt.Freeze();
    m_Deps.State_MapVehi.Freeze();
    m_Deps.State_MapWeap.Freeze();
    m_Deps.State_MapZone.Freeze();

    m_Deps.System_Logs.Log("[TagGroupReader] INFO: Raw built."
        " Bipd loaded: %d | Bloc loaded: %d | Coll loaded: %d |"
        " Ctrl loaded: %d | Eqip loaded: %d | Hlmt loaded: %d |"
        " Lbsp loaded: %d | Mach loaded: %d | Mode loaded: %d |"
        " Phmo loaded: %d | Play loaded: %d | Proj loaded: %d |"
        " Sbsp loaded: %d | Scen loaded: %d | Sncr loaded: %d |"
        " Sldt loaded: %d | Vehi loaded: %d | Weap loaded: %d |"
        " Zone loaded: %d |", bipd, bloc, coll, ctrl, eqip, hlmt, 
        lbsp, mach, mode, phmo, play, proj, sbsp, scen, scnr, 
        sldt, vehi, weap, zone);
}

void System_TagGroupReader::Cleanup()
{
    m_Deps.State_MapBipd.Cleanup();
    m_Deps.State_MapBloc.Cleanup();
    m_Deps.State_MapColl.Cleanup();
    m_Deps.State_MapCtrl.Cleanup();
    m_Deps.State_MapEqip.Cleanup();
    m_Deps.State_MapHlmt.Cleanup();
    m_Deps.State_MapLbsp.Cleanup();
    m_Deps.State_MapMach.Cleanup();
    m_Deps.State_MapMode.Cleanup();
    m_Deps.State_MapPhmo.Cleanup();
    m_Deps.State_MapPlay.Cleanup();
    m_Deps.State_MapProj.Cleanup();
    m_Deps.State_MapSbsp.Cleanup();
    m_Deps.State_MapScen.Cleanup();
    m_Deps.State_MapScnr.Cleanup();
    m_Deps.State_MapSldt.Cleanup();
    m_Deps.State_MapVehi.Cleanup();
    m_Deps.State_MapWeap.Cleanup();
    m_Deps.State_MapZone.Cleanup();

    m_Deps.System_Logs.Log("[TagGroupReader] INFO:"
        " Cleanup completed.");
}