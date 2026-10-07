export module Map.Reader.System:TagGroup;

import :TagResolver;

import Map.Reader.Type;
import std;

export namespace Map::Reader::System
{
    // Describes how to read the tags of one group. Each group specializes it with:
    // DataType (the fixed data of the tag), Magic (the group) and ReadBlocks (the blocks that follow the data).
    template <typename TObject>
    struct GroupDescriptor;

    // Reads tags from the map, with the layout that the GroupDescriptor of their group defines.
    class TagGroupService
    {
    private:
        using TagBlock = Map::Reader::Type::Structure::Tag::Block;

        using TagResolverService = Map::Reader::System::TagResolverService;

    public:
        explicit TagGroupService(TagResolverService& tagResolverService) :
            m_TagResolverService(tagResolverService) {}
        ~TagGroupService() = default;

        // Reads the data of a tag and then its blocks.
        // param tagOffset: File offset of the tag data.
        // return: The object with only the name set if the offset is invalid or the data is cut short.
        template <typename TObject>
        auto Read(std::ifstream& file, std::int64_t tagOffset,
            const std::string& tagName) const -> TObject
        {
            using TData = typename GroupDescriptor<TObject>::DataType;

            TObject object{};
            object.TagName = tagName;
            if (tagOffset < 0) return object;

            file.seekg(tagOffset, std::ios::beg);
            if (!file) return object;

            constexpr std::streamsize dataSize = sizeof(TData);
            file.read(reinterpret_cast<char*>(&object.Data), dataSize);
            if (file.gcount() != dataSize)
            {
                return object;
            }

            GroupDescriptor<TObject>::ReadBlocks(file, *this, object);
            return object;
        }

        // Reads the entries of a block.
        // return: Fewer entries than the block declares if the file is cut short. Empty if the block is empty or has no valid pointer.
        template <typename TEntry>
        auto ReadBlock(std::ifstream& file,
            const TagBlock& block) const -> std::vector<TEntry>
        {
            if (block.EntryCount <= 0) return {};

            std::int64_t offset = m_TagResolverService.ResolveBlockOffset(block);
            if (offset < 0) return {};

            file.seekg(offset, std::ios::beg);
            if (!file) return {};

            std::vector<TEntry> result(block.EntryCount);
            const std::streamsize totalBytes = static_cast<std::streamsize>(block.EntryCount) * sizeof(TEntry);

            file.read(reinterpret_cast<char*>(result.data()), totalBytes);
            if (file.gcount() != totalBytes)
            {
                result.resize(static_cast<std::size_t>(file.gcount() / sizeof(TEntry)));
            }

            return result;
        }

    private:
        TagResolverService& m_TagResolverService;
    };
}