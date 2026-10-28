module Common.Hash.System;

import std;

namespace
{
    // Reflected polynomial of CRC-32.
    constexpr std::uint32_t k_Polynomial{ 0xEDB88320 };
    constexpr std::size_t k_TableSize{ 256 };
    constexpr int k_BitsPerByte{ 8 };

    // Size of the blocks in which a file is read.
    constexpr std::size_t k_BlockSize{ 16 * 1024 * 1024 };

    constexpr auto MakeTable() -> std::array<std::uint32_t, k_TableSize>
    {
        std::array<std::uint32_t, k_TableSize> table{};
        for (std::uint32_t i = 0; i < k_TableSize; ++i)
        {
            std::uint32_t value{ i };
            for (int bit = 0; bit < k_BitsPerByte; ++bit)
            {
                value = (value & 1) ? (value >> 1) ^ k_Polynomial : value >> 1;
            }
            table[i] = value;
        }
        return table;
    }

    constexpr std::array<std::uint32_t, k_TableSize> k_Table{ MakeTable() };
}

namespace Common::Hash::System
{
    auto Crc32(std::span<const std::uint8_t> bytes, std::uint32_t previous) -> std::uint32_t
    {
        std::uint32_t crc{ ~previous };
        for (const std::uint8_t byte : bytes)
        {
            crc = k_Table[(crc ^ byte) & 0xFF] ^ (crc >> k_BitsPerByte);
        }
        return ~crc;
    }

    auto FileCrc32(const std::filesystem::path& path) -> std::optional<std::uint32_t>
    {
        std::ifstream file{ path, std::ios::binary };
        if (!file) return std::nullopt;

        std::vector<std::uint8_t> block(k_BlockSize);
        std::uint32_t crc{};

        while (file)
        {
            file.read(reinterpret_cast<char*>(block.data()), static_cast<std::streamsize>(block.size()));
            const std::streamsize count{ file.gcount() };
            if (count > 0) crc = Crc32(std::span<const std::uint8_t>{ block.data(), static_cast<std::size_t>(count) }, crc);
        }

        if (file.bad()) return std::nullopt;
        return crc;
    }
}