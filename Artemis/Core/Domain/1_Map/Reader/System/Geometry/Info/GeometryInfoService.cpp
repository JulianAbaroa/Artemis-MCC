module Map.Reader.System;
import :Geometry.Info;

namespace Map::Reader::System
{
	auto GeometryInfoService::ReadFooter(const TagResourcesObject& entry,
		std::int32_t& outVBCount, std::int32_t& outIBCount, 
		std::int64_t fixupDataBase) const -> bool
	{
		std::int64_t footerPosition = fixupDataBase +
			entry.FixupInformationOffset +
			entry.FixupInformationLength - m_kFooterSize;

		auto buffer = m_DataStreamService.ReadData(m_FileStore.GetMapFilePath(),
			footerPosition, (std::int32_t)m_kFooterSize);

		if ((std::int64_t)buffer.size() < m_kFooterSize) return false;

		std::memcpy(&outVBCount, buffer.data() + m_kFooterVBCountOffset, sizeof(outVBCount));
		std::memcpy(&outIBCount, buffer.data() + m_kFooterIBCountOffset, sizeof(outIBCount));

		return true;
	}

	auto GeometryInfoService::ReadBufferInfoTables(const TagResourcesObject& entry,
		std::int32_t vbCount, std::int32_t ibCount, 
		std::vector<BufferInfo>& outVBTable, std::vector<BufferInfo>& outIBTable, 
		std::int64_t fixupDataBase) const -> bool
	{
		std::int64_t blockBase = fixupDataBase + entry.FixupInformationOffset;

		std::int64_t vbInfoBytes = (std::int64_t)vbCount * m_kInfoStride;
		std::int64_t auxBytes = (std::int64_t)vbCount * m_kAuxSize;
		std::int64_t ibInfoBytes = (std::int64_t)ibCount * m_kInfoStride;

		std::int64_t totalBytes = vbInfoBytes + auxBytes + ibInfoBytes;
		if (totalBytes <= 0) return false;

		auto infoBuffer = m_DataStreamService.ReadData(
			m_FileStore.GetMapFilePath(), blockBase, (std::int32_t)totalBytes);

		if ((std::int64_t)infoBuffer.size() < totalBytes) return false;

		outVBTable.resize(vbCount);
		for (int i = 0; i < vbCount; ++i)
		{
			const std::uint8_t* pointer = infoBuffer.data() + (std::int64_t)i * m_kInfoStride;

			std::uint32_t vertexCount = 0;
			std::uint32_t dataLength = 0;

			std::memcpy(&vertexCount, pointer + m_kInfoAuxOffset, sizeof(vertexCount));
			std::memcpy(&dataLength, pointer + m_kInfoDataLengthOffset, sizeof(dataLength));

			outVBTable[i] = { dataLength, vertexCount };
		}

		std::int64_t ibBase = vbInfoBytes + auxBytes;

		outIBTable.resize(ibCount);
		for (int i = 0; i < ibCount; ++i)
		{
			const std::uint8_t* pointer = infoBuffer.data() + ibBase + (std::int64_t)i * m_kInfoStride;

			std::uint32_t indexFormat = 0;
			std::uint32_t dataLength = 0;

			std::memcpy(&indexFormat, pointer + m_kInfoAuxOffset, sizeof(indexFormat));
			std::memcpy(&dataLength, pointer + m_kInfoDataLengthOffset, sizeof(dataLength));

			outIBTable[i] = { dataLength, indexFormat };
		}

		return true;
	}
}