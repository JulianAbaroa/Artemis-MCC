export module Template.Object.Type:Raw;

import Resolved.Definitions.Type;
import std;

namespace
{
    using ObjectKind = Resolved::Definitions::Type::Object::ObjectKind;
}

export namespace Template::Object::Type::Raw
{
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
    };

    // Every live object of one read of the object table, by handle.
    struct Snapshot
    {
        std::unordered_map<std::uint32_t, RawObject> RawObjects{};
    };
}