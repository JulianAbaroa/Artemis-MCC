export module Map.Reader.Type:Info;

import std;

export namespace Map::Reader::Type::Info
{
    // Name and group of a tag, resolved from a handle.
    struct TagInfo
    {
        bool IsValid{ false };
        std::string FourCC{};
        std::string TagName{};
    };

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