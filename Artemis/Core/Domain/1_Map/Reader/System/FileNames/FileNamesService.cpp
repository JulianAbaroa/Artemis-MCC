module Map.Reader.System;
import :FileNames;

namespace Map::Reader::System
{
	auto FileNamesService::Read(std::ifstream& file, std::int32_t fileTableCount,
		std::int64_t fileIndexTableOffset, std::int64_t fileTableOffset,
		std::int32_t fileTableSize) const -> bool
	{
		if (fileTableCount <= 0) return true;

		std::uint32_t nameIndexOffset = (std::uint32_t)m_FormulaService.
			ToDebugOffset(fileIndexTableOffset);

		std::uint32_t nameDataOffset = (std::uint32_t)m_FormulaService.
			ToDebugOffset(fileTableOffset);

		m_TagIndexStore.ResizeNameOffsets(fileTableCount);
		file.seekg(nameIndexOffset, std::ios::beg);

		const std::streamsize offsetsByteCount = static_cast<std::streamsize>(fileTableCount) * sizeof(std::int32_t);
		file.read(reinterpret_cast<char*>(m_TagIndexStore.GetNameOffsetsData()), offsetsByteCount);
		if (file.gcount() != offsetsByteCount)
		{
			return false;
		}

		m_TagIndexStore.ResizeNameData(fileTableSize);
		file.seekg(nameDataOffset, std::ios::beg);

		const std::streamsize nameDataByteCount = static_cast<std::streamsize>(fileTableSize);
		file.read(m_TagIndexStore.GetNameData(), nameDataByteCount);
		if (file.gcount() != nameDataByteCount)
		{
			return false;
		}

		return true;
	}
}