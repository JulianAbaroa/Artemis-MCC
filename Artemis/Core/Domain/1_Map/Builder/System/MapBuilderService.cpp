module Map.Builder.System;

import Map.Reader.Type;

namespace
{
    namespace TagTable = Map::Reader::Type::Structure::TagTable;

    using Map::Reader::Type::Magic::Tag::k_Play;
    using Map::Reader::Type::Magic::Tag::k_Zone;
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
        const std::int32_t tagCount = (std::int32_t)m_TagIndexStore.GetTagsSize();

        std::array<std::size_t, 21> counts{};

        for (std::int32_t i = 0; i < tagCount; ++i)
        {
            const TagTable::Entry& entry = m_TagIndexStore.GetTag(i);
            if (entry.TagGroupIndex < 0) continue;

            const std::uint32_t magic = m_TagIndexStore.GetGroupMagic(entry.TagGroupIndex);
            const std::string tagName = m_TagIndexStore.GetTagName(i);

            if (tagName.empty() && magic != k_Play) continue;

            std::size_t slot{};
            m_TagCatalog.ForEach([&]<typename TStore>(TStore& store) {
                using TObject = typename TStore::ObjectType;
                using Descriptor = Map::Reader::System::GroupDescriptor<TObject>;

                if (magic == Descriptor::Magic && !store.Contains(tagName))
                {
                    const std::int64_t offset = m_TagResolverService.GetTagOffset(i);

                    if (offset >= 0)
                    {
                        auto object = std::make_unique<TObject>(
                            reader.Read<TObject>(file, offset, tagName));

                        std::string key = tagName;
                        if constexpr (Descriptor::Magic == k_Play) key = "__play__";
                        if constexpr (Descriptor::Magic == k_Zone) key = "__zone__";

                        store.Add(key, std::move(*object));
                        ++counts[slot];
                    }
                }

                ++slot;
            });
        }

        m_TagCatalog.Freeze();

        m_LogsService.Message("[MapBuilderService] INFO: Raw built."
            " Bipd loaded: {} | Bloc loaded: {} | Coll loaded: {} |"
            " Ctrl loaded: {} | Eqip loaded: {} | Hlmt loaded: {} |"
            " Jpt! loaded: {} | Lbsp loaded: {} | Mach loaded: {} |"
            " Mode loaded: {} | Phmo loaded: {} | Play loaded: {} |"
            " Proj loaded: {} | Sbsp loaded: {} | Scen loaded: {} |"
            " Scnr loaded: {} | Sddt loaded: {} | Sldt loaded: {} |"
            " Vehi loaded: {} | Weap loaded: {} | Zone loaded: {}.",
            counts[0], counts[1], counts[2], counts[3], counts[4], counts[5],
            counts[6], counts[7], counts[8], counts[9], counts[10], counts[11],
            counts[12], counts[13], counts[14], counts[15], counts[16], counts[17],
            counts[18], counts[19], counts[20]);
    }

    auto MapBuilderService::Cleanup() -> void
    {
        m_TagCatalog.Cleanup();

        m_LogsService.Message("[MapBuilderService] INFO: Cleanup completed.");
    }
}