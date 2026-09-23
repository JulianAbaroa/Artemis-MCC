module Map.Reader.System;
import :TagIndex;

import Map.Reader.Type;

namespace
{
	namespace Offset = Map::Reader::Type::Offset;
	namespace Magic = Map::Reader::Type::Magic;

	using TagTableEntry = Map::Reader::Type::Structure::TagTable::Entry;
	using TagTableGroupEntry = Map::Reader::Type::Structure::TagTable::GroupEntry;
}

namespace Map::Reader::System
{
	auto TagIndexService::Read(std::ifstream& file,
		std::int64_t indexHeaderFileOffset) -> bool
	{
		auto seek = [&](std::size_t relativeOffset) {
			file.seekg(indexHeaderFileOffset + static_cast<std::int64_t>(relativeOffset), std::ios::beg);
			return static_cast<bool>(file);
		};

		auto readBytes = [&file](void* dest, std::streamsize bytes) -> bool {
			file.read(reinterpret_cast<char*>(dest), bytes);
			return file.gcount() == bytes;
		};

		std::uint32_t magic = 0;
		if (!seek(Offset::IndexTableHeader::k_Magic) || !readBytes(&magic, sizeof(magic)))
		{
			return false;
		}

		if (magic != Magic::k_IndexHeader)
		{
			return false;
		}

		std::int32_t groupCount = 0;
		if (!seek(Offset::IndexTableHeader::k_NumberOfTagGroups) || !readBytes(&groupCount, sizeof(groupCount)))
		{
			return false;
		}

		std::uint64_t groupTableAddress = 0;
		if (!seek(Offset::IndexTableHeader::k_TagGroupTableAddress) || !readBytes(&groupTableAddress, sizeof(groupTableAddress)))
		{
			return false;
		}

		std::int32_t tagCount = 0;
		if (!seek(Offset::IndexTableHeader::k_NumberOfTags) || !readBytes(&tagCount, sizeof(tagCount)))
		{
			return false;
		}

		std::uint64_t tagTableAddress = 0;
		if (!seek(Offset::IndexTableHeader::k_TagTableAddress) || !readBytes(&tagTableAddress, sizeof(tagTableAddress)))
		{
			return false;
		}

		if (!this->ReadTagGroups(file, m_FormulaService.
			ToFileOffset(static_cast<std::int64_t>(groupTableAddress)), groupCount))
		{
			return false;
		}

		if (!this->ReadTags(file, m_FormulaService.
			ToFileOffset(static_cast<std::int64_t>(tagTableAddress)), tagCount))
		{
			return false;
		}

		return true;
	}

	auto TagIndexService::ReadTagGroups(std::ifstream& file,
		std::int64_t fileOffset, std::int32_t count) -> bool
	{
		if (count <= 0) return false;

		m_TagIndexStore.ResizeGroups(count);

		file.seekg(fileOffset, std::ios::beg);
		if (!file) return false;

		const std::streamsize byteCount = static_cast<std::streamsize>(count) * sizeof(TagTableGroupEntry);
		file.read(reinterpret_cast<char*>(m_TagIndexStore.GetGroupsData()), byteCount);

		return file.gcount() == byteCount;
	}

	auto TagIndexService::ReadTags(std::ifstream& file,
		std::int64_t fileOffset, std::int32_t count) -> bool
	{
		if (count <= 0) return false;

		m_TagIndexStore.ResizeTags(count);

		file.seekg(fileOffset, std::ios::beg);
		if (!file) return false;

		const std::streamsize byteCount = static_cast<std::streamsize>(count) * sizeof(TagTableEntry);
		file.read(reinterpret_cast<char*>(m_TagIndexStore.GetTagsData()), byteCount);

		return file.gcount() == byteCount;
	}
}