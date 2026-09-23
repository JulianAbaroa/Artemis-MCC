export module Map.Reader.System:DataStream;

import std;

export namespace Map::Reader::System
{
	class DataStreamService
	{
	public:
		explicit DataStreamService() = default;
		~DataStreamService() = default;

		auto ReadData(const std::string& filePath, std::int64_t fileOffset, 
			std::int32_t size) const -> std::vector<std::uint8_t>;

		auto ReadSegment(const std::string& filePath, std::int64_t fileOffset, 
			std::int32_t compressedSize, std::int32_t decompressedSize, 
			std::int32_t segmentOffset, std::int32_t segmentLength) const -> 
			std::vector<std::uint8_t>;

	private:
		auto Inflate(const std::vector<std::uint8_t>& compressed,
			std::int32_t compressedSize, std::int32_t decompressedSize) const ->
			std::vector<std::uint8_t>;
	};
}