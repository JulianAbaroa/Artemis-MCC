export module Map.Reader.System:DataStream;

import std;

export namespace Map::Reader::System
{
    // Reads bytes from the map files, inflating them when the block is compressed.
    class DataStreamService
    {
    public:
        explicit DataStreamService() = default;
        ~DataStreamService() = default;

        // Reads a range of bytes from a file.
        // return: The bytes read, which are fewer than size if the file ends first. Empty if the file cannot be read.
        auto ReadData(const std::string& filePath, std::int64_t fileOffset,
            std::int32_t size) const -> std::vector<std::uint8_t>;

        // Reads a slice of a block that is either stored as is or deflate compressed.
        // param fileOffset: File offset where the block starts.
        // param compressedSize: Size of the block in the file. Equal to decompressedSize if it is not compressed.
        // param decompressedSize: Size of the block once inflated.
        // param segmentOffset: Start of the slice inside the inflated block.
        // param segmentLength: Length wanted. It is cut to what is left of the block.
        // return: The slice, or empty if the block cannot be read or inflated.
        auto ReadSegment(const std::string& filePath, std::int64_t fileOffset,
            std::int32_t compressedSize, std::int32_t decompressedSize,
            std::int32_t segmentOffset, std::int32_t segmentLength) const
            -> std::vector<std::uint8_t>;

    private:
        auto Inflate(const std::vector<std::uint8_t>& compressed,
            std::int32_t compressedSize, std::int32_t decompressedSize) const
            -> std::vector<std::uint8_t>;
    };
}