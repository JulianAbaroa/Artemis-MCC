export module Blam.Map.Reader.Type:Info;

import std;

export namespace Blam::Map::Reader::Type::Info
{
    // Values read from the map header that the address conversions depend on.
    struct HeaderInfo
    {
        std::uint64_t VirtualBaseAddress{};
        std::int64_t IndexHeaderFileOffset{};
        std::int32_t FileTableCount{};
        std::int32_t FileTableOffset{};
        std::int32_t FileTableSize{};
        std::int32_t FileIndexTableOffset{};
        std::uint32_t DebugOffsetMask{};
        std::uint32_t ResourceOffsetMask{};
        std::uint32_t TagOffsetMask{};
        std::uint32_t DebugVirtualAddress{};
        std::uint32_t ResourceVirtualAddress{};
        std::uint32_t TagVirtualAddress{};
        std::int64_t TagSectionFileOffset{};
    };
}