export module Map.Reader.State:TagIndex;

import Map.Reader.Type;
import std;

export namespace Map::Reader::State
{
    // Tag index of the loaded map: the tag table, the tag groups and the tag names.
    // note: The data accessors give direct access to the buffers so the readers can fill them. The sizes are thread safe.
    class TagIndexStore
    {
    private:
        using TagTableEntry = Map::Reader::Type::Structure::TagTable::Entry;
        using TagTableGroupEntry = Map::Reader::Type::Structure::TagTable::GroupEntry;

    public:
        TagIndexStore() = default;
        ~TagIndexStore() = default;

        // note: Throws if the index is out of range.
        auto GetTag(std::int32_t index) const -> const TagTableEntry&;

        // return: Magic of the group, or 0 if the index is out of range.
        auto GetGroupMagic(std::int16_t groupIndex) const -> std::uint32_t;

        // return: Empty if the index is out of range, "unknown" if the tag has no name.
        auto GetTagName(std::int32_t index) const -> std::string;

        auto GetTagsData() const -> TagTableEntry*;
        auto GetGroupsData() const -> TagTableGroupEntry*;
        auto GetNameOffsetsData() const -> std::int32_t*;
        auto GetNameData() const -> char*;

        auto GetTagsSize() const -> std::size_t;
        auto GetGroupsSize() const -> std::size_t;
        auto GetNameOffsetsSize() const -> std::size_t;

        auto ResizeTags(std::int32_t count) -> void;
        auto ResizeGroups(std::int32_t count) -> void;
        auto ResizeNameData(std::int32_t count) -> void;
        auto ResizeNameOffsets(std::int32_t count) -> void;

        auto Cleanup() -> void;

    private:
        mutable std::vector<TagTableEntry> m_TagStore{};
        mutable std::vector<TagTableGroupEntry> m_Groups{};
        mutable std::vector<std::int32_t> m_NameOffsets{};
        mutable std::vector<char> m_NameData{};
        mutable std::mutex m_Mutex{};
    };
}