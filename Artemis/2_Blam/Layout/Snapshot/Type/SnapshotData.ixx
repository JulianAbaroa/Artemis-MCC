export module Blam.Layout.Snapshot.Type:Data;

import :Raw;
import :Region;

import Blam.Layout.Object.Type;
import std;

namespace
{
    using Blam::Layout::Object::Type::Constant::k_PlacementSize;
}

export namespace Blam::Layout::Snapshot::Type
{
    // Everything the tick reads from the game, copied at its start: the live objects and the regions of memory.
    // note: Built by the capture on the AI thread and never modified once published. Reads never touch the game.
    struct Snapshot
    {
        // The thread context of the engine was already found.
        bool HasThreadContext{};
        // The object table was already found. Objects can still be empty (no live object).
        bool HasObjectTable{};

        // Every live object by handle.
        std::unordered_map<std::uint32_t, RawObject> RawObjects{};

        // Indexed by RegionId.
        std::array<Region, k_RegionCount> Regions{};

        // return: Whether the region was copied.
        auto IsCaptured(RegionId id) const -> bool
        {
            return !Regions[static_cast<std::size_t>(id)].Bytes.empty();
        }

        // return: The value at offset from the start of the region, or a zero-initialized value if the region is missing or the value does not fit.
        template<typename T>
            requires std::is_trivially_copyable_v<T>
        auto Read(RegionId id, std::size_t offset) const -> T
        {
            T value{};

            const std::vector<std::uint8_t>& bytes{ Regions[static_cast<std::size_t>(id)].Bytes };
            if (offset <= bytes.size() && sizeof(T) <= bytes.size() - offset)
            {
                std::memcpy(&value, bytes.data() + offset, sizeof(T));
            }

            return value;
        }

        // return: The N values starting at offset from the start of the region, or zeros if the region is missing or they do not fit.
        template<typename T, std::size_t N>
            requires std::is_trivially_copyable_v<T>
        auto ReadArray(RegionId id, std::size_t offset) const -> std::array<T, N>
        {
            std::array<T, N> values{};

            const std::vector<std::uint8_t>& bytes{ Regions[static_cast<std::size_t>(id)].Bytes };
            if (offset <= bytes.size() && sizeof(values) <= bytes.size() - offset)
            {
                std::memcpy(values.data(), bytes.data() + offset, sizeof(values));
            }

            return values;
        }

        // return: The value at offset inside the placement, or nothing if the snapshot has no placements or it does not fit in the placement.
        template<typename T>
            requires std::is_trivially_copyable_v<T>
        auto ReadPlacement(std::size_t index, std::size_t offset) const -> std::optional<T>
        {
            if (offset + sizeof(T) > k_PlacementSize) return std::nullopt;

            const std::vector<std::uint8_t>& placements{ Regions[static_cast<std::size_t>(RegionId::Placements)].Bytes };

            const std::size_t start{ (index * k_PlacementSize) + offset };
            if (start + sizeof(T) > placements.size()) return std::nullopt;

            T value{};
            std::memcpy(&value, placements.data() + start, sizeof(T));

            return value;
        }
    };
}