module;

#include <zlib.h>

module Map.Reader.System;
import :DataStream;

namespace Map::Reader::System
{
	auto DataStreamService::ReadData(const std::string& filePath,
		std::int64_t fileOffset, std::int32_t size) const -> 
		std::vector<std::uint8_t>
	{
		if (size <= 0) return {};

		std::ifstream file(filePath, std::ios::binary);
		if (!file.is_open()) return {};

		file.seekg(fileOffset, std::ios::beg);
		if (!file) return {};

		std::vector<std::uint8_t> buffer(size);

		file.read(reinterpret_cast<char*>(buffer.data()), size);

		std::streamsize bytesRead = file.gcount();
		if (bytesRead < static_cast<std::streamsize>(size))
		{
			buffer.resize(static_cast<std::size_t>(bytesRead));
		}

		return buffer;
	}

	auto DataStreamService::ReadSegment(const std::string& filePath,
		std::int64_t fileOffset, std::int32_t compressedSize, 
		std::int32_t decompressedSize, std::int32_t segmentOffset, 
		std::int32_t segmentLength) const -> std::vector<std::uint8_t>
	{
		if (decompressedSize <= 0 || segmentOffset < 0) return {};

		if (compressedSize == decompressedSize)
		{
			std::int32_t length = std::min(segmentLength,
				decompressedSize - segmentOffset);
			if (length <= 0) return {};

			return this->ReadData(filePath,
				fileOffset + segmentOffset, length);
		}

		auto compressed = this->ReadData(filePath, fileOffset, compressedSize);
		if ((std::int32_t)compressed.size() < compressedSize)
		{
			return {};
		}

		auto decompressed = this->Inflate(compressed, compressedSize, decompressedSize);
		if (decompressed.empty()) return {};

		std::int32_t length = std::min(segmentLength,
			decompressedSize - segmentOffset);
		if (length <= 0) return {};

		return std::vector<std::uint8_t>(decompressed.begin() + segmentOffset,
			decompressed.begin() + segmentOffset + length);
	}

	auto DataStreamService::Inflate(const std::vector<std::uint8_t>& compressed,
		std::int32_t compressedSize, std::int32_t decompressedSize) const ->
		std::vector<std::uint8_t>
	{
		std::vector<std::uint8_t> decompressed(decompressedSize);

		z_stream stream{};
		if (inflateInit2(&stream, -15) != Z_OK) return {};

		stream.next_in = const_cast<std::uint8_t*>(compressed.data());
		stream.avail_in = compressedSize;
		stream.next_out = decompressed.data();
		stream.avail_out = decompressedSize;

		int ret = inflate(&stream, Z_FINISH);
		inflateEnd(&stream);

		if (ret != Z_STREAM_END && ret != Z_OK) return {};

		return decompressed;
	}
}