export module Tests.Support.Blam;

import Map.Reader.State;
import Layout.Snapshot.Type;
import std;

namespace
{
    using Map::Reader::State::TagIndexStore;
    using Layout::Snapshot::Type::Region;
    using Layout::Snapshot::Type::RegionId;
    using Layout::Snapshot::Type::Snapshot;
}

export namespace Tests::Support::Blam
{
    // A tag of a tag index written by hand. An empty name is a tag without name (name offset -1).
    struct SyntheticTag
    {
        std::int16_t Group{};
        std::uint16_t Salt{};
        std::string_view Name{};
    };

    // Fills the store the way TagIndexService does: groups, tags, name offsets and the names one after the other.
    auto Fill(TagIndexStore& index, std::span<const std::uint32_t> groups, std::span<const SyntheticTag> tags) -> void
    {
        index.ResizeGroups(static_cast<std::int32_t>(groups.size()));
        for (std::size_t g = 0; g < groups.size(); ++g)
        {
            index.GetGroupsData()[g].Magic = static_cast<std::int32_t>(groups[g]);
        }

        std::string names{};
        std::vector<std::int32_t> offsets{};
        for (const SyntheticTag& tag : tags)
        {
            offsets.push_back(tag.Name.empty() ? -1 : static_cast<std::int32_t>(names.size()));
            if (!tag.Name.empty())
            {
                names += tag.Name;
                names += '\0';
            }
        }

        index.ResizeTags(static_cast<std::int32_t>(tags.size()));
        index.ResizeNameOffsets(static_cast<std::int32_t>(offsets.size()));
        index.ResizeNameData(static_cast<std::int32_t>(names.size()));
        for (std::size_t i = 0; i < tags.size(); ++i)
        {
            index.GetTagsData()[i].TagGroupIndex = tags[i].Group;
            index.GetTagsData()[i].DatumIndexSalt = tags[i].Salt;
            index.GetNameOffsetsData()[i] = offsets[i];
        }
        std::ranges::copy(names, index.GetNameData());
    }

    // return: The handle of the engine for a salt and an index (salt in the high 16 bits).
    auto MakeHandle(std::uint16_t salt, std::uint32_t index) -> std::uint32_t
    {
        return (static_cast<std::uint32_t>(salt) << 16) | (index & 0xFFFF);
    }

    // Writes the bytes of value at offset. The caller sizes the buffer.
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    auto Put(std::vector<std::uint8_t>& bytes, std::size_t offset, const T& value) -> void
    {
        std::memcpy(bytes.data() + offset, &value, sizeof(T));
    }

    // Writes text as UTF-16 code units at offset, without terminator.
    auto PutText(std::vector<std::uint8_t>& bytes, std::size_t offset, std::u16string_view text) -> void
    {
        std::memcpy(bytes.data() + offset, text.data(), text.size() * sizeof(char16_t));
    }

    // Marks the region as captured with size zero bytes, at a made up address.
    // return: The bytes of the region, to fill.
    auto CaptureRegion(Snapshot& snapshot, RegionId id, std::size_t size) -> std::vector<std::uint8_t>&
    {
        Region& region{ snapshot.Regions[static_cast<std::size_t>(id)] };
        region.OriginalAddress = 0x7FF000000000 + (static_cast<std::uintptr_t>(id) << 24);
        region.Bytes.assign(size, 0);
        return region.Bytes;
    }
}