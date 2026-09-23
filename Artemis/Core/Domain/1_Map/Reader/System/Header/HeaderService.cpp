module Map.Reader.System;
import :Header;

namespace
{
	namespace Offset = Map::Reader::Type::Offset;
	using HeaderSection = Map::Reader::Type::Structure::Header::Section;
}

namespace Map::Reader::System
{
	auto HeaderService::Read(std::ifstream& file) -> std::optional<HeaderInfo>
	{
		HeaderInfo headerInfo{};

		auto readBytes = [&file](void* dest, std::streamsize bytes) -> bool {
			file.read(reinterpret_cast<char*>(dest), bytes);
			return file.gcount() == bytes;
		};

		file.seekg(Offset::Header::k_VirtualBaseAddress, std::ios::beg);
		if (!readBytes(&headerInfo.VirtualBaseAddress, sizeof(headerInfo.VirtualBaseAddress)))
		{
			return std::nullopt;
		}

		std::uint64_t indexHeaderAddress = 0;
		if (!readBytes(&indexHeaderAddress, sizeof(indexHeaderAddress)))
		{
			return std::nullopt;
		}

		file.seekg(Offset::Header::k_FileTableCount, std::ios::beg);
		if (!readBytes(&headerInfo.FileTableCount, sizeof(headerInfo.FileTableCount)))
		{
			return std::nullopt;
		}

		if (!readBytes(&headerInfo.FileTableOffset, sizeof(headerInfo.FileTableOffset)))
		{
			return std::nullopt;
		}

		if (!readBytes(&headerInfo.FileTableSize, sizeof(headerInfo.FileTableSize)))
		{
			return std::nullopt;
		}

		if (!readBytes(&headerInfo.FileIndexTableOffset, sizeof(headerInfo.FileIndexTableOffset)))
		{
			return std::nullopt;
		}

		constexpr std::size_t masksCount = Offset::Header::OffsetMasks::k_Count;
		std::uint32_t offsetMasks[masksCount] = {};
		file.seekg(Offset::Header::OffsetMasks::k_Base, std::ios::beg);
		if (!readBytes(offsetMasks, sizeof(offsetMasks)))
		{
			return std::nullopt;
		}

		headerInfo.DebugOffsetMask = offsetMasks[0];
		headerInfo.ResourceOffsetMask = offsetMasks[1];
		headerInfo.TagOffsetMask = offsetMasks[2];

		constexpr std::size_t sectionsCount = Offset::Header::Sections::k_Count;
		HeaderSection sections[sectionsCount] = {};
		file.seekg(Offset::Header::Sections::k_Base, std::ios::beg);
		if (!readBytes(sections, sizeof(sections)))
		{
			return std::nullopt;
		}

		headerInfo.DebugVirtualAddress = sections[0].VirtualAddress;
		headerInfo.ResourceVirtualAddress = sections[1].VirtualAddress;
		headerInfo.TagVirtualAddress = sections[2].VirtualAddress;

		headerInfo.TagSectionFileOffset =
			static_cast<std::int64_t>(headerInfo.TagVirtualAddress) +
			static_cast<std::int32_t>(headerInfo.TagOffsetMask);

		m_FormulaService.Initialize(headerInfo);

		headerInfo.IndexHeaderFileOffset = m_FormulaService.
			ToFileOffset(static_cast<std::int64_t>(indexHeaderAddress));

		return headerInfo;
	}
}