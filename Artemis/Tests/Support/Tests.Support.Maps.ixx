module;

#include <stdlib.h>

export module Tests.Support.Maps;

import std;

namespace
{
    // Folder of the Reach maps in the default Steam install of MCC.
    constexpr std::string_view k_DefaultMapsFolder{
        "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Halo The Master Chief Collection\\haloreach\\maps" };

    constexpr std::string_view k_MapsFolderVariable{ "ARTEMIS_REACH_MAPS" };

    // Reflected polynomial of CRC-32, the same as zlib and Python zlib.crc32.
    constexpr std::uint32_t k_Crc32Polynomial{ 0xEDB88320 };

    constexpr std::size_t k_ChunkSize{ 16 * 1024 * 1024 };

    auto MakeCrc32Table() -> std::array<std::uint32_t, 256>
    {
        std::array<std::uint32_t, 256> table{};
        for (std::uint32_t i = 0; i < 256; ++i)
        {
            std::uint32_t value{ i };
            for (int bit = 0; bit < 8; ++bit)
            {
                value = (value & 1) ? (value >> 1) ^ k_Crc32Polynomial : value >> 1;
            }
            table[i] = value;
        }
        return table;
    }
}

export namespace Tests::Support::Maps
{
    // A Reach cache file that a test reads, with the identity of the copy its expected values were taken from.
    struct ReachFile
    {
        std::string_view Name{};
        std::uintmax_t Size{};
        std::uint32_t Crc32{};
    };

    // Result of looking for a Reach file.
    struct LocatedFile
    {
        // Empty when the file cannot be used.
        std::filesystem::path Path{};

        // Why the file cannot be used, for GTEST_SKIP.
        std::string Reason{};
    };

    // Copies on the PC of the user on 2026-10-23. The expected values of the tests come from the log of the DLL on 2026-10-22
    // with these same files (M3, D181). After an update of MCC: take the new values from the log and change these.
    inline constexpr ReachFile k_ForgeHalo{ .Name = "forge_halo.map", .Size = 332328960, .Crc32 = 0x82384CE0 };
    inline constexpr ReachFile k_Shared{ .Name = "shared.map", .Size = 363823104, .Crc32 = 0xEA17814D };

    // return: ARTEMIS_REACH_MAPS if it is set, else the folder of the default Steam install.
    auto MapsFolder() -> std::filesystem::path
    {
        char* value{ nullptr };
        std::size_t length{};
        std::filesystem::path folder{};

        if (_dupenv_s(&value, &length, k_MapsFolderVariable.data()) == 0 && value != nullptr)
        {
            folder = value;
            std::free(value);
        }

        if (folder.empty()) folder = k_DefaultMapsFolder;
        return folder;
    }

    // return: CRC-32 of the whole file.
    // note: Cached by path for the whole run: each map takes about a second to read.
    auto FileCrc32(const std::filesystem::path& path) -> std::uint32_t
    {
        static std::unordered_map<std::string, std::uint32_t> s_Cache{};
        static const std::array<std::uint32_t, 256> s_Table{ MakeCrc32Table() };

        const std::string key{ path.string() };
        if (auto it = s_Cache.find(key); it != s_Cache.end()) return it->second;

        std::ifstream file{ path, std::ios::binary };
        std::vector<char> buffer(k_ChunkSize);
        std::uint32_t crc{ 0xFFFFFFFF };

        while (file)
        {
            file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize count{ file.gcount() };
            for (std::streamsize i = 0; i < count; ++i)
            {
                crc = s_Table[(crc ^ static_cast<std::uint8_t>(buffer[i])) & 0xFF] ^ (crc >> 8);
            }
        }

        crc ^= 0xFFFFFFFF;
        s_Cache.emplace(key, crc);
        return crc;
    }

    // Finds a Reach file in the maps folder and checks that it is the copy the expected values come from.
    // return: The path, or an empty path and the reason (absent, other size, other CRC-32).
    auto Locate(const ReachFile& file) -> LocatedFile
    {
        const std::filesystem::path path{ MapsFolder() / file.Name };

        std::error_code error{};
        if (!std::filesystem::is_regular_file(path, error))
        {
            return { .Reason = std::format("{} not found (set {} to the folder of the Reach maps).", path.string(),
                k_MapsFolderVariable) };
        }

        const std::uintmax_t size{ std::filesystem::file_size(path, error) };
        if (error || size != file.Size)
        {
            return { .Reason = std::format("{} has {} bytes, the tests expect {} (another version of MCC).", path.string(),
                size, file.Size) };
        }

        const std::uint32_t crc{ FileCrc32(path) };
        if (crc != file.Crc32)
        {
            return { .Reason = std::format("{} has CRC-32 {:08X}, the tests expect {:08X} (another version of MCC).",
                path.string(), crc, file.Crc32) };
        }

        return { .Path = path };
    }
}