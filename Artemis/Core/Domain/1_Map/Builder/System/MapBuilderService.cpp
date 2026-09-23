module Map.Builder.System;

import Map.Reader.Type;

namespace
{
    namespace TagTable = Map::Reader::Type::Structure::TagTable;
    namespace MapMagic = Map::Reader::Type::Magic;
}

namespace Map::Builder::System
{
    auto MapBuilderService::LoadForMap() -> void
    {
        const std::string& filePath = m_FileStore.GetMapFilePath();

        std::ifstream file = m_FileLocatorService.OpenMapFile(filePath);
        if (!file)
        {
            m_LogsService.Message("[MapBuilderService] ERROR: Could not open map file.");
            return;
        }

        TagGroupService reader(m_TagResolverService);

        this->LoadTags(file, reader);

        m_LogsService.Message("[MapBuilderService] INFO: Load completed.");
    }

    auto MapBuilderService::LoadTags(std::ifstream& file, TagGroupService& reader) -> void
    {
        const int32_t tagCount = (int32_t)m_TagIndexStore.GetTagsSize();

        int32_t bipd{}, bloc{}, coll{}, ctrl{}, eqip{}, hlmt{},
            lbsp{}, mach{}, mode{}, phmo{}, play{}, proj{}, sbsp{},
            scen{}, scnr{}, sldt{}, vehi{}, weap{}, zone{};

        for (int32_t i = 0; i < tagCount; ++i)
        {
            const TagTable::Entry& entry = m_TagIndexStore.GetTag(i);
            if (entry.TagGroupIndex < 0) continue;

            const uint32_t magic = m_TagIndexStore.GetGroupMagic(entry.TagGroupIndex);

            std::string tagName = m_TagIndexStore.GetTagName(i);

            if (tagName.empty() && magic != MapMagic::Tag::k_Play) continue;

            // --- Bipd ---
            if (magic == MapMagic::Tag::k_Bipd && !m_BipdStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<BipdObject>(
                        reader.Read<BipdObject>(file, offset, tagName));

                    m_BipdStore.Add(tagName, std::move(*object));
                    ++bipd;
                }
            }

            // --- Bloc ---
            if (magic == MapMagic::Tag::k_Bloc && !m_BlocStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<BlocObject>(
                        reader.Read<BlocObject>(file, offset, tagName));

                    m_BlocStore.Add(tagName, std::move(*object));
                    ++bloc;
                }
            }

            // --- Coll ---
            if (magic == MapMagic::Tag::k_Coll && !m_CollStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<CollObject>(
                        reader.Read<CollObject>(file, offset, tagName));

                    m_CollStore.Add(tagName, std::move(*object));
                    coll++;
                }
            }

            // --- Ctrl ---
            if (magic == MapMagic::Tag::k_Ctrl && !m_CtrlStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<CtrlObject>(
                        reader.Read<CtrlObject>(file, offset, tagName));

                    m_CtrlStore.Add(tagName, std::move(*object));
                    ++ctrl;
                }
            }

            // --- Eqip ---
            if (magic == MapMagic::Tag::k_Eqip && !m_EqipStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<EqipObject>(
                        reader.Read<EqipObject>(file, offset, tagName));

                    m_EqipStore.Add(tagName, std::move(*object));
                    ++eqip;
                }
            }

            // --- Hlmt ---
            if (magic == MapMagic::Tag::k_Hlmt && !m_HlmtStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<HlmtObject>(
                        reader.Read<HlmtObject>(file, offset, tagName));

                    m_HlmtStore.Add(tagName, std::move(*object));
                    ++hlmt;
                }
            }

            // --- Lbsp ---
            if (magic == MapMagic::Tag::k_Lbsp && !m_LbspStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<LbspObject>(
                        reader.Read<LbspObject>(file, offset, tagName));

                    m_LbspStore.Add(tagName, std::move(*object));
                    ++lbsp;
                }
            }

            // --- Mode ---
            if (magic == MapMagic::Tag::k_Mode && !m_ModeStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<ModeObject>(
                        reader.Read<ModeObject>(file, offset, tagName));

                    m_ModeStore.Add(tagName, std::move(*object));
                    mode++;
                }
            }

            // --- Phmo ---
            if (magic == MapMagic::Tag::k_Phmo && !m_PhmoStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<PhmoObject>(
                        reader.Read<PhmoObject>(file, offset, tagName));

                    m_PhmoStore.Add(tagName, std::move(*object));
                    ++phmo;
                }
            }

            // --- Play ---
            if (magic == MapMagic::Tag::k_Play && !m_PlayStore.Contains(tagName))
            {
                std::string tagName = m_TagIndexStore.GetTagName(i);
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<PlayObject>(
                        reader.Read<PlayObject>(file, offset, tagName));

                    tagName = "__play__";
                    m_PlayStore.Add(tagName, std::move(*object));
                    ++play;
                }
            }

            // --- Proj ---
            if (magic == MapMagic::Tag::k_Proj && !m_ProjStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<ProjObject>(
                        reader.Read<ProjObject>(file, offset, tagName));

                    m_ProjStore.Add(tagName, std::move(*object));
                    ++proj;
                }
            }

            // --- Sbsp ---
            if (magic == MapMagic::Tag::k_Sbsp && !m_SbspStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<SbspObject>(
                        reader.Read<SbspObject>(file, offset, tagName));

                    m_SbspStore.Add(tagName, std::move(*object));
                    ++sbsp;
                }
            }

            // --- Scen ---
            if (magic == MapMagic::Tag::k_Scen && !m_ScenStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<ScenObject>(
                        reader.Read<ScenObject>(file, offset, tagName));

                    m_ScenStore.Add(tagName, std::move(*object));
                    ++scen;
                }
            }

            // --- Scnr ---
            if (magic == MapMagic::Tag::k_Scnr && !m_ScnrStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<ScnrObject>(
                        reader.Read<ScnrObject>(file, offset, tagName));

                    m_ScnrStore.Add(tagName, std::move(*object));
                    ++scnr;
                }

            }

            // --- Sldt ---
            if (magic == MapMagic::Tag::k_SLdT && !m_SldtStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<SldtObject>(
                        reader.Read<SldtObject>(file, offset, tagName));

                    m_SldtStore.Add(tagName, std::move(*object));
                    ++sldt;
                }
            }

            // --- Vehi ---
            if (magic == MapMagic::Tag::k_Vehi && !m_VehiStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<VehiObject>(
                        reader.Read<VehiObject>(file, offset, tagName));

                    m_VehiStore.Add(tagName, std::move(*object));
                    ++vehi;
                }
            }

            // --- Weap ---
            if (magic == MapMagic::Tag::k_Weap && !m_WeapStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<WeapObject>(
                        reader.Read<WeapObject>(file, offset, tagName));

                    m_WeapStore.Add(tagName, std::move(*object));
                    ++weap;
                }
            }

            // --- Zone ---
            if (magic == MapMagic::Tag::k_Zone && !m_ZoneStore.Contains(tagName))
            {
                const int64_t offset = m_TagResolverService.GetTagOffset(i);

                if (offset >= 0)
                {
                    auto object = std::make_unique<ZoneObject>(
                        reader.Read<ZoneObject>(file, offset, tagName));

                    tagName = "__zone__";
                    m_ZoneStore.Add(tagName, std::move(*object));
                    ++zone;
                }
            }
        }

        m_BipdStore.Freeze();
        m_BlocStore.Freeze();
        m_CollStore.Freeze();
        m_CtrlStore.Freeze();
        m_EqipStore.Freeze();
        m_HlmtStore.Freeze();
        m_LbspStore.Freeze();
        m_MachStore.Freeze();
        m_PlayStore.Freeze();
        m_ModeStore.Freeze();
        m_PhmoStore.Freeze();
        m_ProjStore.Freeze();
        m_SbspStore.Freeze();
        m_ScenStore.Freeze();
        m_ScnrStore.Freeze();
        m_SldtStore.Freeze();
        m_VehiStore.Freeze();
        m_WeapStore.Freeze();
        m_ZoneStore.Freeze();

        m_LogsService.Message("[MapBuilderService] INFO: Raw built."
            " Bipd loaded: {} | Bloc loaded: {} | Coll loaded: {} |"
            " Ctrl loaded: {} | Eqip loaded: {} | Hlmt loaded: {} |"
            " Lbsp loaded: {} | Mach loaded: {} | Mode loaded: {} |"
            " Phmo loaded: {} | Play loaded: {} | Proj loaded: {} |"
            " Sbsp loaded: {} | Scen loaded: {} | Sncr loaded: {} |"
            " Sldt loaded: {} | Vehi loaded: {} | Weap loaded: {} |"
            " Zone loaded: {} |", bipd, bloc, coll, ctrl, eqip, hlmt,
            lbsp, mach, mode, phmo, play, proj, sbsp, scen, scnr,
            sldt, vehi, weap, zone);
    }

    auto MapBuilderService::Cleanup() -> void
    {
        m_BipdStore.Cleanup();
        m_BlocStore.Cleanup();
        m_CollStore.Cleanup();
        m_CtrlStore.Cleanup();
        m_EqipStore.Cleanup();
        m_HlmtStore.Cleanup();
        m_LbspStore.Cleanup();
        m_MachStore.Cleanup();
        m_ModeStore.Cleanup();
        m_PhmoStore.Cleanup();
        m_PlayStore.Cleanup();
        m_ProjStore.Cleanup();
        m_SbspStore.Cleanup();
        m_ScenStore.Cleanup();
        m_ScnrStore.Cleanup();
        m_SldtStore.Cleanup();
        m_VehiStore.Cleanup();
        m_WeapStore.Cleanup();
        m_ZoneStore.Cleanup();

        m_LogsService.Message("[MapBuilderService] INFO: Cleanup completed.");
    }
}