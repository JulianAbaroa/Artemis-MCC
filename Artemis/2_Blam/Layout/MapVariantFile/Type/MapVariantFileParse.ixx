export module Blam.Layout.MapVariantFile.Type:Parse;

import std;

namespace
{
    // return: The big endian integer of the bytes at an offset.
    template <typename T>
    auto ReadBigEndian(std::span<const std::byte> bytes, std::size_t offset) -> T
    {
        T value{ 0 };
        for (std::size_t index{ 0 }; index < sizeof(T); ++index)
        {
            value = static_cast<T>((value << 8) | static_cast<std::uint8_t>(bytes[offset + index]));
        }
        return value;
    }

    // return: The code units of a UTF-16 little endian field of the file, up to the first 0 or the end of the field.
    auto ReadText(std::span<const std::byte> bytes, std::size_t offset, std::size_t characters) -> std::u16string
    {
        std::u16string text{};

        for (std::size_t index{ 0 }; index < characters; ++index)
        {
            const auto low{ static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes[offset + index * 2])) };
            const auto high{ static_cast<std::uint16_t>(static_cast<std::uint8_t>(bytes[offset + index * 2 + 1])) };
            const auto unit{ static_cast<char16_t>(low | (high << 8)) };

            if (unit == u'\0') break;

            text.push_back(unit);
        }

        return text;
    }
}

// Pure reading of the map variant files of Forge (.mvar, Q106): a BLF file made of chunks, with the name and description of the map variant
// in its content header. It works on bytes it is given and never touches the disk or the game.
export namespace Blam::Layout::MapVariantFile::Type::Parse
{
    // Tags of the chunks (four characters, big endian) and the bytes of a chunk header: tag, size (header included), major and minor.
    inline constexpr std::uint32_t k_FileTag{ 0x5F626C66 };
    inline constexpr std::uint32_t k_HeaderTag{ 0x63686472 };
    inline constexpr std::uint32_t k_MapVariantTag{ 0x6D766172 };
    inline constexpr std::uint32_t k_EndTag{ 0x5F656F66 };
    inline constexpr std::size_t k_ChunkHeaderSize{ 12 };

    // Smallest first chunk, and where it holds the byte order mark (bytes FF FE in either order, as the engine accepts).
    inline constexpr std::size_t k_FileChunkMinSize{ 0x30 };
    inline constexpr std::size_t k_ByteOrderOffset{ 0xC };

    // Name and description of the content header, as UTF-16 little endian fields from the start of its chunk.
    inline constexpr std::size_t k_NameOffset{ 0x90 };
    inline constexpr std::size_t k_TextCharacters{ 128 };
    inline constexpr std::size_t k_DescriptionOffset{ k_NameOffset + k_TextCharacters * 2 };
    inline constexpr std::size_t k_HeaderChunkMinSize{ k_DescriptionOffset + k_TextCharacters * 2 };

    // Size of the content header chunk, and the bytes from the start of a file that hold its first two chunks: all the catalogue of the
    // files needs to read (D291).
    inline constexpr std::size_t k_HeaderChunkSize{ k_HeaderChunkMinSize + 0x30 };
    inline constexpr std::size_t k_CatalogBytes{ k_FileChunkMinSize + k_HeaderChunkSize };

    // Bytes of a map variant chunk the engine decode reads at least: the bitstream ends at +0x7024.
    inline constexpr std::size_t k_MapVariantChunkMinSize{ 0x7024 };

    // What a .mvar says about itself.
    struct MapVariantFile
    {
        // UTF-16 as the file holds them: the callers that may import Common convert them.
        std::u16string Name{};
        std::u16string Description{};
        // The map variant chunk, header included, inside the file. Size 0 when the bytes given do not reach it.
        std::size_t MvarOffset{ 0 };
        std::size_t MvarSize{ 0 };
    };

    struct MapVariantFileResult
    {
        MapVariantFile File{};
        // Why the bytes are not a map variant file. Empty when they are.
        std::string Error{};

        auto IsOk() const -> bool
        {
            return Error.empty();
        }
    };

    // Walks the chunks of a file, skipping the ones it does not know, and reads the name and description of the content header.
    // param bytes: The file, or its first k_CatalogBytes (then the map variant chunk is not reached and MvarSize is 0).
    // return: The file, or the reason it cannot be read: not a BLF file, wrong byte order, a chunk too small or cut short, no content header.
    auto Read(std::span<const std::byte> bytes) -> MapVariantFileResult
    {
        MapVariantFileResult result{};
        const std::size_t total{ bytes.size() };
        std::size_t position{ 0 };
        bool isHeaderRead{ false };

        if (total < k_ChunkHeaderSize || ReadBigEndian<std::uint32_t>(bytes, 0) != k_FileTag)
        {
            result.Error = "not a BLF file (no _blf chunk)";
            return result;
        }

        while (total - position >= k_ChunkHeaderSize)
        {
            const auto tag{ ReadBigEndian<std::uint32_t>(bytes, position) };
            const auto size{ static_cast<std::size_t>(ReadBigEndian<std::uint32_t>(bytes, position + 4)) };

            if (size < k_ChunkHeaderSize)
            {
                result.Error = std::format("chunk at 0x{:X} has size {}", position, size);
                return result;
            }

            if (size > total - position)
            {
                result.Error = std::format("chunk at 0x{:X} is cut short ({} of {} bytes)", position, total - position, size);
                return result;
            }

            if (tag == k_FileTag)
            {
                const bool isOrderValid{ position + k_ByteOrderOffset + 2 <= total &&
                    ((bytes[position + k_ByteOrderOffset] == std::byte{ 0xFF } && bytes[position + k_ByteOrderOffset + 1] == std::byte{ 0xFE }) ||
                     (bytes[position + k_ByteOrderOffset] == std::byte{ 0xFE } && bytes[position + k_ByteOrderOffset + 1] == std::byte{ 0xFF })) };

                if (size < k_FileChunkMinSize || !isOrderValid)
                {
                    result.Error = "the _blf chunk is too small or has no byte order mark";
                    return result;
                }
            }
            else if (tag == k_HeaderTag)
            {
                if (size < k_HeaderChunkMinSize)
                {
                    result.Error = std::format("the chdr chunk is too small ({} bytes)", size);
                    return result;
                }

                result.File.Name = ReadText(bytes, position + k_NameOffset, k_TextCharacters);
                result.File.Description = ReadText(bytes, position + k_DescriptionOffset, k_TextCharacters);
                isHeaderRead = true;
            }
            else if (tag == k_MapVariantTag)
            {
                result.File.MvarOffset = position;
                result.File.MvarSize = size;
            }
            else if (tag == k_EndTag)
            {
                break;
            }

            position += size;
        }

        if (!isHeaderRead) result.Error = "no chdr chunk";

        return result;
    }

    // A run of bytes that differ between two blocks.
    struct DiffRange
    {
        std::size_t Offset{ 0 };
        std::size_t Size{ 0 };
    };

    struct BlockDiff
    {
        // Bytes that differ, counting the ones one block has and the other lacks.
        std::size_t Bytes{ 0 };
        // The runs, in order of offset.
        std::vector<DiffRange> Ranges{};
    };

    // Compares two blocks byte by byte. The tail of the longer one counts as different.
    auto Diff(std::span<const std::byte> first, std::span<const std::byte> second) -> BlockDiff
    {
        BlockDiff diff{};
        const std::size_t common{ (std::min)(first.size(), second.size()) };
        const std::size_t longest{ (std::max)(first.size(), second.size()) };

        for (std::size_t index{ 0 }; index < longest; ++index)
        {
            const bool isDifferent{ index >= common || first[index] != second[index] };

            if (!isDifferent) continue;

            ++diff.Bytes;

            if (!diff.Ranges.empty() && diff.Ranges.back().Offset + diff.Ranges.back().Size == index) ++diff.Ranges.back().Size;
            else diff.Ranges.push_back(DiffRange{ .Offset = index, .Size = 1 });
        }

        return diff;
    }
}