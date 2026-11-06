export module Tests.Support.Blam;

import Blam.Map.Reader.State;
import Blam.Layout.Snapshot.Type;
import std;

namespace
{
    using ::Blam::Map::Reader::State::TagIndexStore;
    using ::Blam::Layout::Snapshot::Type::RawObject;
    using ::Blam::Layout::Snapshot::Type::Region;
    using ::Blam::Layout::Snapshot::Type::RegionId;
    using ::Blam::Layout::Snapshot::Type::Snapshot;
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

    // Block of player traits of ctf_c without objective (the base traits, A19): speed 110%, gravity 75%, jump 100%, DMR, AR, sprint.
    inline constexpr std::array<std::uint8_t, 0x2C> k_CtfCBaseTraits{
        0x04, 0x02, 0x07, 0x02, 0x0A, 0x0A, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x06, 0x06, 0x01, 0x02,
        0x0C, 0x02, 0x02, 0x01, 0x02, 0x00, 0x01, 0x00, 0x07, 0x02, 0x08, 0x01, 0x64, 0x00, 0x00, 0x00,
        0x01, 0x02, 0x03, 0x01, 0x01, 0x00, 0x00, 0x00, 0x03, 0x03, 0x02, 0x00 };

    // return: The same block with the trait set of the flag carrier of ctf_c on top (A19): shield vampirism 100%, melee 125%, ability
    // usage disabled, infinite abilities off, speed 90%, no driver, jump 150%, waypoint everyone, forced color, radar 75 m.
    constexpr auto CarrierTraits() -> std::array<std::uint8_t, 0x2C>
    {
        std::array<std::uint8_t, 0x2C> block{ k_CtfCBaseTraits };
        block[0x07] = 5;
        block[0x0D] = 8;
        block[0x12] = 1;
        block[0x14] = 1;
        block[0x18] = 5;
        block[0x1A] = 6;
        block[0x1C] = 150;
        block[0x21] = 3;
        block[0x24] = 10;
        block[0x29] = 5;
        return block;
    }

    // An object with the bytes it owns, to build the objects of a Snapshot. PutObject copies it into the table of the Snapshot.
    struct OwnedObject
    {
        std::uint32_t Handle{};
        std::uintptr_t Address{};
        std::uint8_t KindId{};
        std::vector<std::uint8_t> Bytes{};

        // return: The view of the bytes of this object, valid as long as it is not changed.
        auto View() const -> RawObject
        {
            return RawObject{ .Handle = Handle, .Address = Address, .KindId = KindId, .Bytes = Bytes };
        }
    };

    // Adds the object to the snapshot, with a copy of its bytes.
    auto PutObject(Snapshot& snapshot, const OwnedObject& object) -> void
    {
        snapshot.RawObjects.Add(object.Handle, object.Address, object.KindId, object.Bytes);
    }

    // Writes the bytes of value at offset. The caller sizes the buffer.
    template <typename T>
        requires std::is_trivially_copyable_v<T>
    auto Put(std::span<std::uint8_t> bytes, std::size_t offset, const T& value) -> void
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