export module Datum.Object.Type:Raw;

import Resolved.Definitions.Type;
import std;

namespace
{
    using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
}

export namespace Datum::Object::Type::Raw
{
    // Position of a variable block inside the datum.
    struct BlockRange
    {
        std::size_t Offset{};
        std::size_t Size{};
    };

    // Copy of the whole datum of a live object (fixed part and variable blocks), as it was in memory.
    struct RawObject
    {
        std::uint32_t Handle{};
        std::uintptr_t Address{};
        ObjectKind Kind{ ObjectKind::Invalid };
        std::vector<std::uint8_t> Bytes{};

        // return: The value at offset, or a zero-initialized value if it does not fit in the datum.
        template<typename T> 
            requires std::is_trivially_copyable_v<T>
        auto Read(std::size_t offset) const -> T
        {
            T value{};

            if (offset + sizeof(T) <= Bytes.size())
            {
                std::memcpy(&value, Bytes.data() + offset, sizeof(T));
            }

            return value;
        }

        // Copies size bytes from offset into destination.
        // return: False if the range does not fit in the datum.
        auto ReadRaw(std::size_t offset, void* destination, std::size_t size) const -> bool
        {
            if (offset + size > Bytes.size()) return false;

            std::memcpy(destination, Bytes.data() + offset, size);
            return true;
        }

        // Finds a variable block through its descriptor, which is { u16 size, u16 offset from the datum start }.
        // return: The range of the block, or nothing if the object does not have it or it does not fit in the datum.
        // note: The position of a block changes with the tag of the object, so it is read from the descriptor every time.
        auto FindBlock(std::size_t descriptorOffset) const -> std::optional<BlockRange>
        {
            const std::size_t size{ this->Read<std::uint16_t>(descriptorOffset) };
            const std::size_t offset{ this->Read<std::uint16_t>(descriptorOffset + 2) };

            if (size == 0 || offset == 0xFFFF) return std::nullopt;
            if (offset + size > Bytes.size()) return std::nullopt;

            return BlockRange{ .Offset = offset, .Size = size };
        }

        // return: The value at offset inside the block, or a zero-initialized value if the block is missing or the value does not fit in it.
        template<typename T>
            requires std::is_trivially_copyable_v<T>
        auto ReadBlock(std::size_t descriptorOffset, std::size_t offset) const -> T
        {
            const std::optional<BlockRange> block{ this->FindBlock(descriptorOffset) };
            if (!block || offset + sizeof(T) > block->Size) return T{};

            return this->Read<T>(block->Offset + offset);
        }
    };

    // Every live object of one read of the object table, by handle.
    struct Snapshot
    {
        std::unordered_map<std::uint32_t, RawObject> RawObjects{};
    };
}