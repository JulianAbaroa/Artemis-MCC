export module Map.Reader.System:TagGroup;

import :TagResolver;

import Map.Reader.Type;
import std;

export namespace Map::Reader::System
{
    template <typename TObject>
    struct GroupDescriptor;

    class TagGroupService
    {
    private:
        using TagBlock = Map::Reader::Type::Structure::Tag::Block;

        using TagResolverService = Map::Reader::System::TagResolverService;

    public:
        explicit TagGroupService(TagResolverService& tagResolverService) :
            m_TagResolverService(tagResolverService) {}
        ~TagGroupService() = default;

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