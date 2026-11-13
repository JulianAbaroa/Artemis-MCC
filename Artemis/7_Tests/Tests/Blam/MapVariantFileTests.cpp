#include <gtest/gtest.h>

import Blam.Layout.MapVariantFile.Type;
import std;

// The pure reading of the .mvar files of Forge (Q106): the chunks, the name and description, the map variant chunk and the diff of two
// blocks. The synthetic files are built here, and the two real files of the fixtures fix the layout the engine decode reads.

namespace
{
    namespace FileParse = Blam::Layout::MapVariantFile::Type::Parse;

    using FileParse::DiffRange;

    using FileParse::k_ChunkHeaderSize;
    using FileParse::k_EndTag;
    using FileParse::k_FileChunkMinSize;
    using FileParse::k_FileTag;
    using FileParse::k_HeaderChunkSize;
    using FileParse::k_HeaderTag;
    using FileParse::k_MapVariantTag;
    using FileParse::k_NameOffset;
    using FileParse::k_DescriptionOffset;
    using FileParse::k_TextCharacters;

    using Bytes = std::vector<std::byte>;

    // Appends the header of a chunk (big endian) and zeros up to its size.
    auto AppendChunk(Bytes& file, std::uint32_t tag, std::uint32_t size) -> std::size_t
    {
        const std::size_t start{ file.size() };

        for (const std::uint32_t value : { tag, size })
        {
            for (int shift{ 24 }; shift >= 0; shift -= 8) file.push_back(static_cast<std::byte>((value >> shift) & 0xFF));
        }

        file.resize(start + size, std::byte{ 0 });
        return start;
    }

    auto WriteText(Bytes& file, std::size_t offset, std::u16string_view text) -> void
    {
        for (std::size_t index{ 0 }; index < text.size(); ++index)
        {
            file[offset + index * 2] = static_cast<std::byte>(text[index] & 0xFF);
            file[offset + index * 2 + 1] = static_cast<std::byte>(text[index] >> 8);
        }
    }

    // return: A file with the _blf chunk, the content header with the texts, an unknown chunk and a map variant chunk of mvarSize bytes.
    auto MakeFile(std::u16string_view name, std::u16string_view description, std::uint32_t mvarSize) -> Bytes
    {
        Bytes file{};
        const std::size_t blf{ AppendChunk(file, k_FileTag, static_cast<std::uint32_t>(k_FileChunkMinSize)) };
        file[blf + 0xC] = std::byte{ 0xFF };
        file[blf + 0xD] = std::byte{ 0xFE };

        const std::size_t header{ AppendChunk(file, k_HeaderTag, static_cast<std::uint32_t>(k_HeaderChunkSize)) };
        WriteText(file, header + k_NameOffset, name);
        WriteText(file, header + k_DescriptionOffset, description);

        if (mvarSize != 0) AppendChunk(file, k_MapVariantTag, mvarSize);

        AppendChunk(file, k_EndTag, 0x11);
        return file;
    }

    auto Span(const Bytes& bytes) -> std::span<const std::byte>
    {
        return bytes;
    }

    auto ReadFixture(std::string_view name) -> Bytes
    {
        // This file is Artemis/7_Tests/Tests/Blam/MapVariantFileTests.cpp.
        const std::filesystem::path source{ std::source_location::current().file_name() };
        std::ifstream file{ source.parent_path() / "Fixtures" / "mvar" / name, std::ios::binary };
        const std::vector<char> text{ std::istreambuf_iterator<char>{ file }, std::istreambuf_iterator<char>{} };
        return Bytes{ reinterpret_cast<const std::byte*>(text.data()), reinterpret_cast<const std::byte*>(text.data() + text.size()) };
    }
}

TEST(MapVariantFileTests, ReadsNameDescriptionAndTheMapVariantChunk)
{
    const Bytes file{ MakeFile(u"Test Map", u"A description", 0x7028) };
    const auto result{ FileParse::Read(Span(file)) };

    ASSERT_TRUE(result.IsOk()) << result.Error;
    EXPECT_EQ(result.File.Name, u"Test Map");
    EXPECT_EQ(result.File.Description, u"A description");
    EXPECT_EQ(result.File.MvarOffset, k_FileChunkMinSize + k_HeaderChunkSize);
    EXPECT_EQ(result.File.MvarSize, 0x7028u);
}

TEST(MapVariantFileTests, EmptyNameGivesAnEmptyText)
{
    const Bytes file{ MakeFile(u"", u"", 0x7028) };
    const auto result{ FileParse::Read(Span(file)) };

    ASSERT_TRUE(result.IsOk()) << result.Error;
    EXPECT_TRUE(result.File.Name.empty());
}

TEST(MapVariantFileTests, ANameWithoutTerminatorIsCutAtTheField)
{
    const Bytes file{ MakeFile(std::u16string(k_TextCharacters + 10, u'x'), u"", 0) };
    const auto result{ FileParse::Read(Span(file)) };

    ASSERT_TRUE(result.IsOk()) << result.Error;
    EXPECT_EQ(result.File.Name.size(), k_TextCharacters);
}

TEST(MapVariantFileTests, TheFirstBytesAreEnoughForTheCatalog)
{
    const Bytes file{ MakeFile(u"Only Header", u"", 0x7028) };
    const auto result{ FileParse::Read(Span(file).first(FileParse::k_CatalogBytes)) };

    ASSERT_TRUE(result.IsOk()) << result.Error;
    EXPECT_EQ(result.File.Name, u"Only Header");
    EXPECT_EQ(result.File.MvarSize, 0u);
}

TEST(MapVariantFileTests, ASwappedByteOrderMarkIsAccepted)
{
    Bytes file{ MakeFile(u"A", u"", 0) };
    file[0xC] = std::byte{ 0xFE };
    file[0xD] = std::byte{ 0xFF };

    EXPECT_TRUE(FileParse::Read(Span(file)).IsOk());
}

TEST(MapVariantFileTests, RejectsWhatIsNotABlfFile)
{
    Bytes file{ MakeFile(u"A", u"", 0) };
    file[0] = std::byte{ 'X' };

    const auto result{ FileParse::Read(Span(file)) };

    EXPECT_FALSE(result.IsOk());
    EXPECT_NE(result.Error.find("not a BLF file"), std::string::npos);
    EXPECT_FALSE(FileParse::Read({}).IsOk());
}

TEST(MapVariantFileTests, RejectsAWrongByteOrderMark)
{
    Bytes file{ MakeFile(u"A", u"", 0) };
    file[0xC] = std::byte{ 0 };

    EXPECT_FALSE(FileParse::Read(Span(file)).IsOk());
}

TEST(MapVariantFileTests, RejectsATruncatedChunk)
{
    Bytes file{ MakeFile(u"A", u"", 0x7028) };
    file.resize(k_FileChunkMinSize + k_HeaderChunkSize + 0x100);

    const auto result{ FileParse::Read(Span(file)) };

    EXPECT_FALSE(result.IsOk());
    EXPECT_NE(result.Error.find("cut short"), std::string::npos);
}

TEST(MapVariantFileTests, RejectsAChunkSmallerThanItsHeader)
{
    Bytes file{ MakeFile(u"A", u"", 0) };
    file[k_FileChunkMinSize + 7] = std::byte{ 4 };
    file[k_FileChunkMinSize + 6] = std::byte{ 0 };
    file[k_FileChunkMinSize + 5] = std::byte{ 0 };
    file[k_FileChunkMinSize + 4] = std::byte{ 0 };

    const auto result{ FileParse::Read(Span(file)) };

    EXPECT_FALSE(result.IsOk());
    EXPECT_NE(result.Error.find("has size 4"), std::string::npos);
}

TEST(MapVariantFileTests, SkipsAnUnknownChunk)
{
    Bytes file{};
    const std::size_t blf{ AppendChunk(file, k_FileTag, static_cast<std::uint32_t>(k_FileChunkMinSize)) };
    file[blf + 0xC] = std::byte{ 0xFF };
    file[blf + 0xD] = std::byte{ 0xFE };
    AppendChunk(file, 0x78787878, 0x40);
    const std::size_t header{ AppendChunk(file, k_HeaderTag, static_cast<std::uint32_t>(k_HeaderChunkSize)) };
    WriteText(file, header + k_NameOffset, u"After Unknown");

    const auto result{ FileParse::Read(Span(file)) };

    ASSERT_TRUE(result.IsOk()) << result.Error;
    EXPECT_EQ(result.File.Name, u"After Unknown");
}

TEST(MapVariantFileTests, RejectsAFileWithoutContentHeader)
{
    Bytes file{};
    const std::size_t blf{ AppendChunk(file, k_FileTag, static_cast<std::uint32_t>(k_FileChunkMinSize)) };
    file[blf + 0xC] = std::byte{ 0xFF };
    file[blf + 0xD] = std::byte{ 0xFE };
    AppendChunk(file, k_EndTag, 0x11);

    const auto result{ FileParse::Read(Span(file)) };

    EXPECT_FALSE(result.IsOk());
    EXPECT_EQ(result.Error, "no chdr chunk");
}

TEST(MapVariantFileTests, RealFilesGiveTheirNameAndTheMapVariantChunk)
{
    const Bytes first{ ReadFixture("ctf_1v1_first.mvar") };
    const Bytes second{ ReadFixture("ctf_2v2_final.mvar") };

    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());

    const auto firstResult{ FileParse::Read(Span(first)) };
    const auto secondResult{ FileParse::Read(Span(second)) };

    ASSERT_TRUE(firstResult.IsOk()) << firstResult.Error;
    ASSERT_TRUE(secondResult.IsOk()) << secondResult.Error;
    EXPECT_EQ(firstResult.File.Name, u"CTF (1v1) First");
    EXPECT_EQ(secondResult.File.Name, u"CTF (2v2) Final");
    EXPECT_EQ(firstResult.File.MvarOffset, 0x2F0u);
    EXPECT_EQ(firstResult.File.MvarSize, 0x7028u);
    EXPECT_GE(firstResult.File.MvarSize, FileParse::k_MapVariantChunkMinSize);
    EXPECT_EQ(secondResult.File.MvarOffset, 0x2F0u);
}

TEST(MapVariantFileTests, DiffCountsBytesAndMergesRuns)
{
    const Bytes first(16, std::byte{ 0 });
    Bytes second{ first };
    second[2] = std::byte{ 1 };
    second[3] = std::byte{ 1 };
    second[9] = std::byte{ 1 };

    const auto diff{ FileParse::Diff(Span(first), Span(second)) };

    EXPECT_EQ(diff.Bytes, 3u);
    ASSERT_EQ(diff.Ranges.size(), 2u);
    EXPECT_EQ(diff.Ranges[0].Offset, 2u);
    EXPECT_EQ(diff.Ranges[0].Size, 2u);
    EXPECT_EQ(diff.Ranges[1].Offset, 9u);
    EXPECT_EQ(diff.Ranges[1].Size, 1u);
}

TEST(MapVariantFileTests, DiffOfEqualBlocksIsEmptyAndALongerTailDiffers)
{
    const Bytes block(8, std::byte{ 7 });
    Bytes longer{ block };
    longer.resize(12, std::byte{ 7 });

    EXPECT_EQ(FileParse::Diff(Span(block), Span(block)).Bytes, 0u);

    const auto diff{ FileParse::Diff(Span(block), Span(longer)) };

    EXPECT_EQ(diff.Bytes, 4u);
    ASSERT_EQ(diff.Ranges.size(), 1u);
    EXPECT_EQ(diff.Ranges[0].Offset, 8u);
}