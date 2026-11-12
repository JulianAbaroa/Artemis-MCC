export module Blam.Layout.Object.Type:MapVariant;

import :Constant;
import :Mvar.Offset;

import std;

namespace
{
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelCountOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelOffsetsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelStringsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelStringsSize;
    using Blam::Layout::Object::Type::Constant::k_MaxLabels;
    using Blam::Layout::Object::Type::Constant::k_NoLabel;
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantMapIdOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantQuotasOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersionOffset;
    using Blam::Layout::Object::Type::Constant::k_MaxPlacements;
    using Blam::Layout::Object::Type::Constant::k_PlacementSize;
    using Blam::Layout::Object::Type::Constant::k_PlacementsOffset;
    using Blam::Layout::Object::Type::Mvar::Offset::k_GametypeLabel;
    using Blam::Layout::Object::Type::Mvar::Offset::k_PlacementFlags;

    // Bit of the placement flags that marks a slot holding a placement.
    constexpr std::uint16_t k_PlacementInUse{ 0x1 };

    template <typename T>
    auto WriteValue(std::span<std::byte> block, std::size_t offset, T value) -> void
    {
        std::memcpy(block.data() + offset, &value, sizeof(T));
    }

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

    // The incoming block with its labels in the order the match already has them, and what was done to get there.
    struct LabelRemap
    {
        std::vector<std::byte> Block{};
        // Labels of the incoming block whose index changed.
        std::uint32_t Moved{ 0 };
        // Labels of the incoming block that the current one did not have: they went after its labels.
        std::uint32_t Appended{ 0 };
        // Labels of the current block that the incoming one does not use and that stay in the table.
        std::uint32_t Kept{ 0 };
    };

    // return: The labels of a block, by index, or the reason the table is not valid (count, offset or a string without its zero).
    auto ReadLabels(std::span<const std::byte> block) -> std::expected<std::vector<std::string>, std::string>
    {
        const std::int32_t count{ ReadValue<std::int32_t>(block, k_MapVariantLabelCountOffset) };
        if (count < 0 || static_cast<std::size_t>(count) > k_MaxLabels) return std::unexpected(std::format("{} labels", count));

        std::vector<std::string> labels{};
        for (std::size_t index{ 0 }; index < static_cast<std::size_t>(count); ++index)
        {
            const std::size_t offset{ ReadValue<std::uint16_t>(block, k_MapVariantLabelOffsetsOffset + index * sizeof(std::uint16_t)) };
            if (offset >= k_MapVariantLabelStringsSize) return std::unexpected(std::format("label {} starts at {}, outside its area", index, offset));

            const auto* first{ reinterpret_cast<const char*>(block.data() + k_MapVariantLabelStringsOffset + offset) };
            const std::size_t room{ k_MapVariantLabelStringsSize - offset };
            const std::size_t length{ std::string_view{ first, room }.find('\0') };
            if (length == std::string_view::npos) return std::unexpected(std::format("label {} has no end zero", index));

            labels.emplace_back(first, length);
        }

        return labels;
    }

    // return: The incoming map variant written so that the labels the match already has keep their index (D266, LS.2). The Megalo lists of
    // objects by label are indexed by the label index the load built, and a different order mixes the chains of two labels up. The labels of
    // the current block keep their index and their string, the labels of the incoming one that are not in it (exact string compare) go after
    // them, the string area, the offsets and the count are rebuilt and every placement in use is pointed to its label through the old to
    // new index (k_NoLabel stays, and an index the incoming table does not have becomes k_NoLabel). A label of the current block that the incoming one
    // does not use stays in the table. When the order is already the same the block is returned as it came.
    // Fails, writing nothing, when a table is not valid or the merged one has more than k_MaxLabels labels or more strings than the area holds.
    // param current: The map variant the engine runs now (k_MapVariantSize bytes).
    // param incoming: The map variant to write (k_MapVariantSize bytes).
    auto RemapLabels(std::span<const std::byte> current, std::span<const std::byte> incoming) -> std::expected<LabelRemap, std::string>
    {
        if (current.size() < k_MapVariantSize || incoming.size() < k_MapVariantSize) return std::unexpected("a map variant is too small");

        const auto currentLabels{ ReadLabels(current) };
        if (!currentLabels) return std::unexpected("current labels: " + currentLabels.error());
        const auto incomingLabels{ ReadLabels(incoming) };
        if (!incomingLabels) return std::unexpected("incoming labels: " + incomingLabels.error());

        LabelRemap result{ .Block = std::vector<std::byte>(incoming.begin(), incoming.begin() + k_MapVariantSize) };

        std::vector<std::string> merged{ *currentLabels };
        std::vector<std::size_t> moved{};
        for (std::size_t index{ 0 }; index < incomingLabels->size(); ++index)
        {
            const auto found{ std::ranges::find(merged, (*incomingLabels)[index]) };
            if (found == merged.end())
            {
                merged.push_back((*incomingLabels)[index]);
                ++result.Appended;
                moved.push_back(merged.size() - 1);
            }
            else
            {
                moved.push_back(static_cast<std::size_t>(found - merged.begin()));
            }

            if (moved.back() != index) ++result.Moved;
        }

        result.Kept = static_cast<std::uint32_t>(std::ranges::count_if(*currentLabels,
            [&](const std::string& label) { return std::ranges::find(*incomingLabels, label) == incomingLabels->end(); }));

        if (merged == *incomingLabels) return result;

        std::size_t bytes{ 0 };
        for (const std::string& label : merged) bytes += label.size() + 1;
        if (merged.size() > k_MaxLabels) return std::unexpected(std::format("{} labels do not fit in {}", merged.size(), k_MaxLabels));
        if (bytes > k_MapVariantLabelStringsSize) return std::unexpected(std::format("labels of {} bytes do not fit in {}", bytes, k_MapVariantLabelStringsSize));

        std::span<std::byte> block{ result.Block };
        std::ranges::fill(block.subspan(k_MapVariantLabelStringsOffset, k_MapVariantLabelStringsSize), std::byte{ 0 });
        std::ranges::fill(block.subspan(k_MapVariantLabelOffsetsOffset, k_MaxLabels * sizeof(std::uint16_t)), std::byte{ 0 });

        std::size_t offset{ 0 };
        for (std::size_t index{ 0 }; index < merged.size(); ++index)
        {
            std::memcpy(block.data() + k_MapVariantLabelStringsOffset + offset, merged[index].data(), merged[index].size());
            WriteValue<std::uint16_t>(block, k_MapVariantLabelOffsetsOffset + index * sizeof(std::uint16_t), static_cast<std::uint16_t>(offset));
            offset += merged[index].size() + 1;
        }

        WriteValue<std::int32_t>(block, k_MapVariantLabelCountOffset, static_cast<std::int32_t>(merged.size()));

        for (std::uint32_t index{ 0 }; index < k_MaxPlacements; ++index)
        {
            const std::size_t placement{ k_PlacementsOffset + index * k_PlacementSize };
            if ((ReadValue<std::uint16_t>(block, placement + k_PlacementFlags) & k_PlacementInUse) == 0) continue;

            const std::uint16_t label{ ReadValue<std::uint16_t>(block, placement + k_GametypeLabel) };
            if (label == k_NoLabel) continue;

            // An index the incoming table does not have would point at an unrelated label of the merged one: it has none.
            WriteValue<std::uint16_t>(block, placement + k_GametypeLabel, label < moved.size() ? static_cast<std::uint16_t>(moved[label]) : k_NoLabel);
        }

        return result;
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