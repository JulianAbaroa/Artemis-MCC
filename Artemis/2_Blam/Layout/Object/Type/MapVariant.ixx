export module Blam.Layout.Object.Type:MapVariant;

import :Constant;
import :Mvar.Offset;

import std;

namespace
{
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantMapIdOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantQuotasOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersionOffset;
    using Blam::Layout::Object::Type::Constant::k_MaxPlacements;
    using Blam::Layout::Object::Type::Constant::k_PlacementSize;
    using Blam::Layout::Object::Type::Constant::k_PlacementsOffset;
    using Blam::Layout::Object::Type::Mvar::Offset::k_PlacementFlags;

    // Bit of the placement flags that marks a slot holding a placement.
    constexpr std::uint16_t k_PlacementInUse{ 0x1 };

    template <typename T>
    auto ReadValue(std::span<const std::byte> block, std::size_t offset) -> T
    {
        T value{};
        std::memcpy(&value, block.data() + offset, sizeof(T));
        return value;
    }
}

// Pure logic of the map variants (Q98, D266): read the header of a map variant block, decide whether it can replace the one the engine runs
// and compose the block written into the runtime variant. It works on bytes it is given and never touches the game.
export namespace Blam::Layout::Object::Type::MapVariant
{
    // Fields of the header of a map variant that decide whether it fits the loaded map.
    struct MapVariantHeader
    {
        std::uint16_t Version{};
        std::uint16_t Quotas{};
        std::uint32_t MapId{};
        std::uint32_t PlacementsInUse{};

        auto operator==(const MapVariantHeader&) const -> bool = default;
    };

    // return: The header of a map variant block, or nothing if the block has fewer than k_MapVariantSize bytes.
    // note: PlacementsInUse counts the placements with the In Use flag, over the whole placement array.
    auto Read(std::span<const std::byte> block) -> std::optional<MapVariantHeader>
    {
        if (block.size() < k_MapVariantSize) return std::nullopt;

        MapVariantHeader header{
            .Version = ReadValue<std::uint16_t>(block, k_MapVariantVersionOffset),
            .Quotas = ReadValue<std::uint16_t>(block, k_MapVariantQuotasOffset),
            .MapId = ReadValue<std::uint32_t>(block, k_MapVariantMapIdOffset),
        };

        for (std::uint32_t index{ 0 }; index < k_MaxPlacements; ++index)
        {
            const std::size_t placement{ k_PlacementsOffset + index * k_PlacementSize };

            if ((ReadValue<std::uint16_t>(block, placement + k_PlacementFlags) & k_PlacementInUse) != 0) ++header.PlacementsInUse;
        }

        return header;
    }

    // return: Nothing if the new map variant can replace the current one, otherwise the reason it cannot: version other than
    // k_MapVariantVersion, other map id (the palette indices would belong to another map) or another number of quotas.
    auto CheckCompatible(const MapVariantHeader& incoming, const MapVariantHeader& current) -> std::optional<std::string>
    {
        if (incoming.Version != k_MapVariantVersion)
        {
            return std::format("version 0x{:X} instead of 0x{:X}", incoming.Version, k_MapVariantVersion);
        }

        if (incoming.MapId != current.MapId)
        {
            return std::format("map id {} instead of {}", incoming.MapId, current.MapId);
        }

        if (incoming.Quotas != current.Quotas)
        {
            return std::format("{} quotas instead of {}", incoming.Quotas, current.Quotas);
        }

        return std::nullopt;
    }

    // return: The runtime variant: the new block with the world bounds of the current one (the engine recomputed them at load). Empty if
    // either block has fewer than k_MapVariantSize bytes.
    auto ComposeRuntime(std::span<const std::byte> incoming, std::span<const std::byte> current) -> std::vector<std::byte>
    {
        if (incoming.size() < k_MapVariantSize || current.size() < k_MapVariantSize) return {};

        std::vector<std::byte> runtime(incoming.begin(), incoming.begin() + k_MapVariantSize);
        std::memcpy(runtime.data() + k_MapVariantBoundsOffset, current.data() + k_MapVariantBoundsOffset, k_MapVariantBoundsSize);
        return runtime;
    }
}