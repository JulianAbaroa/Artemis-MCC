#include <gtest/gtest.h>

import Common.Hash.System;
import std;

// Crc32 and FileCrc32 of Common (M5c, D203): the same CRC-32 as zlib, to identify the map a recording was made with.

namespace
{
    using Common::Hash::System::Crc32;
    using Common::Hash::System::FileCrc32;

    // return: The bytes of the text, without terminator.
    auto Bytes(std::string_view text) -> std::vector<std::uint8_t>
    {
        return std::vector<std::uint8_t>{ text.begin(), text.end() };
    }
}

TEST(HashTests, Crc32OfTheCheckString)
{
    // The check value of CRC-32 (zlib.crc32(b"123456789")).
    EXPECT_EQ(Crc32(Bytes("123456789")), 0xCBF43926u);
}

TEST(HashTests, ChainedCrc32EqualsOneCall)
{
    const std::vector<std::uint8_t> whole{ Bytes("123456789") };
    const std::span<const std::uint8_t> bytes{ whole };

    const std::uint32_t first{ Crc32(bytes.first(4)) };
    EXPECT_EQ(Crc32(bytes.subspan(4), first), Crc32(bytes));
}

TEST(HashTests, EmptyIsZero)
{
    EXPECT_EQ(Crc32(std::span<const std::uint8_t>{}), 0u);
}

TEST(HashTests, FileCrc32EqualsCrc32OfItsBytes)
{
    const std::filesystem::path path{ std::filesystem::temp_directory_path() / "artemis_hash_test.bin" };

    // More than one byte pattern period, so the table is exercised with every index.
    std::vector<std::uint8_t> bytes(1000);
    for (std::size_t i{ 0 }; i < bytes.size(); ++i) bytes[i] = static_cast<std::uint8_t>(i * 7);

    {
        std::ofstream file{ path, std::ios::binary | std::ios::trunc };
        file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    }

    const std::optional<std::uint32_t> crc{ FileCrc32(path) };
    std::error_code error{};
    std::filesystem::remove(path, error);

    ASSERT_TRUE(crc.has_value());
    EXPECT_EQ(*crc, Crc32(bytes));
}

TEST(HashTests, FileCrc32OfAMissingFileIsNothing)
{
    const std::filesystem::path path{ std::filesystem::temp_directory_path() / "artemis_hash_test_missing" / "none.bin" };

    EXPECT_EQ(FileCrc32(path), std::nullopt);
}