export module Map.Reader.State:TagIndex;

import Map.Reader.Type;
import std;

export namespace Map::Reader::State
{
	class TagIndexStore
	{
	private:
		using TagTableEntry = Map::Reader::Type::Structure::TagTable::Entry;
		using TagTableGroupEntry = Map::Reader::Type::Structure::TagTable::GroupEntry;

	public:
		TagIndexStore() = default;
		~TagIndexStore() = default;

		auto GetTag(std::int32_t index) const -> const TagTableEntry&;
		auto GetGroupMagic(std::int16_t groupIndex) const -> std::uint32_t;
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