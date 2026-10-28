export module Common.Hash.System;

import std;

export namespace Common::Hash::System
{
    // Computes the CRC-32 (reflected polynomial 0xEDB88320, the same as zlib.crc32) of the bytes.
    // param previous: The CRC-32 of the bytes before these, to chain calls. 0 starts a new one.
    auto Crc32(std::span<const std::uint8_t> bytes, std::uint32_t previous = 0) -> std::uint32_t;

    // Computes the CRC-32 of a whole file, read in blocks.
    // return: The CRC-32, or nullopt when the file cannot be read.
    auto FileCrc32(const std::filesystem::path& path) -> std::optional<std::uint32_t>;
}