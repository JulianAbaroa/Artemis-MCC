export module Map.Reader.System:Formula;

import Map.Reader.Type;
import std;

export namespace Map::Reader::System
{
    // Converts the addresses stored in the map into offsets in the map file.
    // note: Initialize must be called with the header values before any conversion.
    class FormulaService
    {
    private:
        using HeaderInfo = Map::Reader::Type::Info::HeaderInfo;

    public:
        FormulaService() = default;
        ~FormulaService() = default;

        auto Initialize(HeaderInfo headerInfo) -> void;
        auto Cleanup() -> void;

        // Turns a stored tag address into a virtual address.
        // return: 0 if the address is null or invalid.
        auto Expand(std::uint32_t address) const -> std::int64_t;

        // Turns a virtual address of the tag section into a file offset.
        auto ToFileOffset(std::int64_t virtualAddress) const -> std::int64_t;

        // Turns a pointer into the debug section, such as the tag names, into a file offset.
        auto ToDebugOffset(std::int64_t pointer) const -> std::int64_t;

        // Turns an offset into the resource section into a file offset.
        auto ToResourceOffset(std::int64_t blockOffset) const -> std::int64_t;

        // Builds the offset of a resource fixup from the parts of its address.
        // param address: Lower 16 bits.
        // param addressUpperBits: Next 16 bits.
        // param addressLocationHighBits: Highest bits, from bit 24.
        // return: The offset inside the fixup data, masked to 28 bits.
        auto ResolveFixupOffset(std::uint16_t address, std::uint16_t addressUpperBits,
            std::uint8_t addressLocationHighBits) const -> std::uint32_t;

    private:
        HeaderInfo m_HeaderInfo{};
    };
}