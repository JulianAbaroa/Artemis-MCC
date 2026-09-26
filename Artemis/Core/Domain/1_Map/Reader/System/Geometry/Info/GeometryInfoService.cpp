module Map.Reader.System;
import :Geometry.Info;

namespace
{
	using Map::Reader::Type::Constant::k_FooterSize;
	using Map::Reader::Type::Constant::k_FooterVBCountOffset;
	using Map::Reader::Type::Constant::k_FooterIBCountOffset;
	using Map::Reader::Type::Constant::k_InfoStride;
	using Map::Reader::Type::Constant::k_AuxSize;
	using Map::Reader::Type::Constant::k_InfoAuxOffset;
	using Map::Reader::Type::Constant::k_InfoDataLengthOffset;
}

namespace Map::Reader::System
{
	auto GeometryInfoService::ReadFooter(const TagResourcesObject& entry,
		std::int32_t& outVBCount, std::int32_t& outIBCount, 
		std::int64_t fixupDataBase) const -> bool
	{
		std::int64_t footerPosition = fixupDataBase +
			entry.FixupInformationOffset +
			entry.FixupInformationLength - k_FooterSize;

		auto buffer = m_DataStreamService.ReadData(m_FileStore.GetMapFilePath(),
			footerPosition, (std::int32_t)k_FooterSize);

		if ((std::int64_t)buffer.size() < k_FooterSize) return false;

		std::memcpy(&outVBCount, buffer.data() + k_FooterVBCountOffset, sizeof(outVBCount));
		std::memcpy(&outIBCount, buffer.data() + k_FooterIBCountOffset, sizeof(outIBCount));

		return true;
	}

	auto GeometryInfoService::ReadBufferInfoTables(const TagResourcesObject& entry,
		std::int32_t vbCount, std::int32_t ibCount, 
		std::vector<BufferInfo>& outVBTable, std::vector<BufferInfo>& outIBTable, 
		std::int64_t fixupDataBase) const -> bool
	{
		std::int64_t blockBase = fixupDataBase + entry.FixupInformationOffset;

		std::int64_t vbInfoBytes = (std::int64_t)vbCount * k_InfoStride;
		std::int64_t auxBytes = (std::int64_t)vbCount * k_AuxSize;
		std::int64_t ibInfoBytes = (std::int64_t)ibCount * k_InfoStride;

		std::int64_t totalBytes = vbInfoBytes + auxBytes + ibInfoBytes;
		if (totalBytes <= 0) return false;

		auto infoBuffer = m_DataStreamService.ReadData(
			m_FileStore.GetMapFilePath(), blockBase, (std::int32_t)totalBytes);

		if ((std::int64_t)infoBuffer.size() < totalBytes) return false;

		outVBTable.resize(vbCount);
		for (int i = 0; i < vbCount; ++i)
		{
			const std::uint8_t* pointer = infoBuffer.data() + (std::int64_t)i * k_InfoStride;

			std::uint32_t vertexCount = 0;
			std::uint32_t dataLength = 0;

			std::memcpy(&vertexCount, pointer + k_InfoAuxOffset, sizeof(vertexCount));
			std::memcpy(&dataLength, pointer + k_InfoDataLengthOffset, sizeof(dataLength));

			outVBTable[i] = { dataLength, vertexCount };
		}

		std::int64_t ibBase = vbInfoBytes + auxBytes;

		outIBTable.resize(ibCount);
		for (int i = 0; i < ibCount; ++i)
		{
			const std::uint8_t* pointer = infoBuffer.data() + ibBase + (std::int64_t)i * k_InfoStride;

			std::uint32_t indexFormat = 0;
			std::uint32_t dataLength = 0;

			std::memcpy(&indexFormat, pointer + k_InfoAuxOffset, sizeof(indexFormat));
			std::memcpy(&dataLength, pointer + k_InfoDataLengthOffset, sizeof(dataLength));

			outIBTable[i] = { dataLength, indexFormat };
		}

		return true;
	}
}