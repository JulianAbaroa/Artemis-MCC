#include <gtest/gtest.h>

import Blam.Layout.Object.Type;
import std;

// The pure logic of the map variants (Q98, D266): header read, compatibility check and runtime composition, on synthetic blocks.

namespace
{
    using Blam::Layout::Object::Type::MapVariant::CheckCompatible;
    using Blam::Layout::Object::Type::MapVariant::ComposeRuntime;
    using Blam::Layout::Object::Type::MapVariant::MapVariantHeader;
    using Blam::Layout::Object::Type::MapVariant::ReadLabels;
    using Blam::Layout::Object::Type::MapVariant::RemapLabels;
    using Blam::Layout::Object::Type::MapVariant::Read;

    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantBoundsSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelCountOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelOffsetsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelStringsOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantLabelStringsSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantMapIdOffset;
    using Blam::Layout::Object::Type::Constant::k_MaxLabels;
    using Blam::Layout::Object::Type::Constant::k_NoLabel;
    using Blam::Layout::Object::Type::Mvar::Offset::k_GametypeLabel;
    using Blam::Layout::Object::Type::Constant::k_MapVariantQuotasOffset;
    using Blam::Layout::Object::Type::Constant::k_MapVariantSize;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersion;
    using Blam::Layout::Object::Type::Constant::k_MapVariantVersionOffset;
    using Blam::Layout::Object::Type::Constant::k_MaxPlacements;
    using Blam::Layout::Object::Type::Constant::k_PlacementSize;
    using Blam::Layout::Object::Type::Constant::k_PlacementsOffset;

    using Block = std::vector<std::byte>;

    template <typename T>
    auto Put(Block& block, std::size_t offset, T value) -> void
    {
        std::memcpy(block.data() + offset, &value, sizeof(T));
    }

    // return: A map variant block: header fields as given, placements in use for the first count slots, bytes of the rest set to fill.
    auto MakeBlock(std::uint16_t version, std::uint16_t quotas, std::uint32_t mapId, std::uint32_t placements, std::uint8_t fill) -> Block
    {
        Block block(k_MapVariantSize, static_cast<std::byte>(fill));
        Put<std::uint16_t>(block, k_MapVariantVersionOffset, version);
        Put<std::uint16_t>(block, k_MapVariantQuotasOffset, quotas);
        Put<std::uint32_t>(block, k_MapVariantMapIdOffset, mapId);

        for (std::uint32_t index{ 0 }; index < k_MaxPlacements; ++index)
        {
            Put<std::uint16_t>(block, k_PlacementsOffset + index * k_PlacementSize, index < placements ? 0x1 : 0x0);
        }

        return block;
    }
}

TEST(MapVariantTests, ReadGivesTheHeader)
{
    const Block block{ MakeBlock(k_MapVariantVersion, 145, 3006, 462, 0x11) };

    const auto header{ Read(block) };

    ASSERT_TRUE(header.has_value());
    EXPECT_EQ(header->Version, k_MapVariantVersion);
    EXPECT_EQ(header->Quotas, 145);
    EXPECT_EQ(header->MapId, 3006u);
    EXPECT_EQ(header->PlacementsInUse, 462u);
}

TEST(MapVariantTests, ReadRejectsAShortBlock)
{
    const Block block(k_MapVariantSize - 1, std::byte{ 0 });

    EXPECT_FALSE(Read(block).has_value());
    EXPECT_FALSE(Read({}).has_value());
}

TEST(MapVariantTests, ReadCountsOnlyTheInUseFlag)
{
    Block block{ MakeBlock(k_MapVariantVersion, 145, 3006, 0, 0) };
    Put<std::uint16_t>(block, k_PlacementsOffset, 0xFFFE);
    Put<std::uint16_t>(block, k_PlacementsOffset + k_PlacementSize, 0x0003);

    EXPECT_EQ(Read(block)->PlacementsInUse, 1u);
}

TEST(MapVariantTests, CompatibleWhenVersionMapAndQuotasMatch)
{
    const MapVariantHeader current{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006, .PlacementsInUse = 462 };
    const MapVariantHeader incoming{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006, .PlacementsInUse = 651 };

    EXPECT_FALSE(CheckCompatible(incoming, current).has_value());
}

TEST(MapVariantTests, DifferentMapIdGivesTheReason)
{
    const MapVariantHeader current{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006 };
    MapVariantHeader incoming{ current };
    incoming.MapId = 3010;

    const auto reason{ CheckCompatible(incoming, current) };

    ASSERT_TRUE(reason.has_value());
    EXPECT_NE(reason->find("map id 3010 instead of 3006"), std::string::npos);
}

TEST(MapVariantTests, WrongVersionGivesTheReason)
{
    const MapVariantHeader current{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006 };
    MapVariantHeader incoming{ current };
    incoming.Version = 0x1F;

    const auto reason{ CheckCompatible(incoming, current) };

    ASSERT_TRUE(reason.has_value());
    EXPECT_NE(reason->find("version 0x1F instead of 0x20"), std::string::npos);
}

TEST(MapVariantTests, DifferentQuotasGiveTheReason)
{
    const MapVariantHeader current{ .Version = k_MapVariantVersion, .Quotas = 145, .MapId = 3006 };
    MapVariantHeader incoming{ current };
    incoming.Quotas = 120;

    const auto reason{ CheckCompatible(incoming, current) };

    ASSERT_TRUE(reason.has_value());
    EXPECT_NE(reason->find("120 quotas instead of 145"), std::string::npos);
}

TEST(MapVariantTests, ComposeRuntimeKeepsOnlyTheWorldBounds)
{
    const Block incoming{ MakeBlock(k_MapVariantVersion, 145, 3006, 651, 0xAA) };
    const Block current{ MakeBlock(k_MapVariantVersion, 145, 3006, 462, 0xBB) };

    const Block runtime{ ComposeRuntime(incoming, current) };

    ASSERT_EQ(runtime.size(), k_MapVariantSize);

    for (std::size_t offset{ 0 }; offset < k_MapVariantSize; ++offset)
    {
        const bool isBounds{ offset >= k_MapVariantBoundsOffset && offset < k_MapVariantBoundsOffset + k_MapVariantBoundsSize };
        const Block& source{ isBounds ? current : incoming };

        ASSERT_EQ(runtime[offset], source[offset]) << "offset 0x" << std::hex << offset;
    }

    EXPECT_EQ(Read(runtime)->PlacementsInUse, 651u);
}

TEST(MapVariantTests, ComposeRuntimeRejectsAShortBlock)
{
    const Block full{ MakeBlock(k_MapVariantVersion, 145, 3006, 1, 0) };
    const Block shortBlock(k_MapVariantSize - 1, std::byte{ 0 });

    EXPECT_TRUE(ComposeRuntime(shortBlock, full).empty());
    EXPECT_TRUE(ComposeRuntime(full, shortBlock).empty());
}


// --- Label remap (LS.2, D266): the labels of the match keep their index when a map variant is written between rounds ---------------

namespace
{
    // Writes the label table of a block as the engine does: strings one after the other, their offsets and the count.
    auto SetLabels(Block& block, const std::vector<std::string>& labels) -> void
    {
        std::size_t offset{ 0 };
        for (std::size_t index{ 0 }; index < labels.size(); ++index)
        {
            std::memcpy(block.data() + k_MapVariantLabelStringsOffset + offset, labels[index].c_str(), labels[index].size() + 1);
            Put<std::uint16_t>(block, k_MapVariantLabelOffsetsOffset + index * 2, static_cast<std::uint16_t>(offset));
            offset += labels[index].size() + 1;
        }

        Put<std::int32_t>(block, k_MapVariantLabelCountOffset, static_cast<std::int32_t>(labels.size()));
    }

    // Points a slot at a label. The slot is in use when it is below the count of placements the block was made with.
    auto SetLabel(Block& block, std::size_t slot, std::uint16_t label) -> void
    {
        Put<std::uint16_t>(block, k_PlacementsOffset + slot * k_PlacementSize + k_GametypeLabel, label);
    }

    auto LabelOf(const Block& block, std::size_t slot) -> std::uint16_t
    {
        std::uint16_t value{};
        std::memcpy(&value, block.data() + k_PlacementsOffset + slot * k_PlacementSize + k_GametypeLabel, sizeof(value));
        return value;
    }

    // return: A map variant with labels and 4 placements in use that point at the labels 0, 1, none and 1 (first label, second label...).
    auto MakeLabeled(const std::vector<std::string>& labels, std::initializer_list<std::uint16_t> placements) -> Block
    {
        Block block{ MakeBlock(k_MapVariantVersion, 145, 3006, static_cast<std::uint32_t>(placements.size()), 0) };
        SetLabels(block, labels);
        std::size_t slot{ 0 };
        for (const std::uint16_t label : placements) SetLabel(block, slot++, label);
        return block;
    }
}

TEST(MapVariantRemapTests, TheSameLabelsInTheSameOrderLeaveTheBlockAsItCame)
{
    const Block current{ MakeLabeled({ "teleport", "flag_stand" }, { 0, 1 }) };
    const Block incoming{ MakeLabeled({ "teleport", "flag_stand" }, { 1, 1, 0 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    EXPECT_EQ(remap->Block, incoming);
    EXPECT_EQ(remap->Moved, 0u);
    EXPECT_EQ(remap->Appended, 0u);
}

TEST(MapVariantRemapTests, FirstToOpenFlagMovesFlagStandAndAppendsTheOthers)
{
    // The labels the user's configs have: CTF 1v1 First [teleport, flag_stand] and Open Flag 6v6 [flag_stand, teleporter, blink_spawn, ...].
    const Block current{ MakeLabeled({ "teleport", "flag_stand" }, { 0, 1 }) };
    const Block incoming{ MakeLabeled({ "flag_stand", "teleporter", "blink_spawn", "bottom", "harlan_version" }, { 0, 1, 2, 3, 4, 0 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    const auto labels{ ReadLabels(remap->Block) };
    ASSERT_TRUE(labels.has_value());
    EXPECT_EQ(*labels, (std::vector<std::string>{ "teleport", "flag_stand", "teleporter", "blink_spawn", "bottom", "harlan_version" }));
    // flag_stand 0 -> 1, the rest are appended after the labels the match has.
    EXPECT_EQ(LabelOf(remap->Block, 0), 1);
    EXPECT_EQ(LabelOf(remap->Block, 1), 2);
    EXPECT_EQ(LabelOf(remap->Block, 2), 3);
    EXPECT_EQ(LabelOf(remap->Block, 3), 4);
    EXPECT_EQ(LabelOf(remap->Block, 4), 5);
    EXPECT_EQ(LabelOf(remap->Block, 5), 1);
    EXPECT_EQ(remap->Moved, 5u);
    EXPECT_EQ(remap->Appended, 4u);
    EXPECT_EQ(remap->Kept, 1u);
}

TEST(MapVariantRemapTests, AppendedLabelsKeepTheIndexOfTheExistingOnes)
{
    // First -> Ground Pass [teleport, flag_stand, harlan_version]: same indices, one label appended.
    const Block current{ MakeLabeled({ "teleport", "flag_stand" }, { 0, 1 }) };
    const Block incoming{ MakeLabeled({ "teleport", "flag_stand", "harlan_version" }, { 0, 1, 2 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    EXPECT_EQ(remap->Block, incoming);
    EXPECT_EQ(remap->Moved, 0u);
    EXPECT_EQ(remap->Appended, 1u);
}

TEST(MapVariantRemapTests, ALabelCurrentHasThatTheIncomingDoesNotUseStaysInTheTable)
{
    const Block current{ MakeLabeled({ "a", "b", "c" }, { 0 }) };
    const Block incoming{ MakeLabeled({ "c", "a" }, { 0, 1 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    EXPECT_EQ(*ReadLabels(remap->Block), (std::vector<std::string>{ "a", "b", "c" }));
    EXPECT_EQ(LabelOf(remap->Block, 0), 2);
    EXPECT_EQ(LabelOf(remap->Block, 1), 0);
    EXPECT_EQ(remap->Kept, 1u);
    EXPECT_EQ(remap->Appended, 0u);
}

TEST(MapVariantRemapTests, NoLabelStaysAndUnusedPlacementsAreNotTouched)
{
    const Block current{ MakeLabeled({ "teleport", "flag_stand" }, { 0 }) };
    Block incoming{ MakeLabeled({ "flag_stand", "teleporter" }, { k_NoLabel, 0, 1 }) };
    // A slot that is not in use keeps its label field as it is, even if it holds an index the table has.
    SetLabel(incoming, 10, 1);
    SetLabel(incoming, 11, 7);

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    EXPECT_EQ(LabelOf(remap->Block, 0), k_NoLabel);
    EXPECT_EQ(LabelOf(remap->Block, 1), 1);
    EXPECT_EQ(LabelOf(remap->Block, 2), 2);
    EXPECT_EQ(LabelOf(remap->Block, 10), 1);
    EXPECT_EQ(LabelOf(remap->Block, 11), 7);
}

TEST(MapVariantRemapTests, ThePlacementsAndTheRestOfTheBlockAreNotChangedApartFromTheLabelTable)
{
    const Block current{ MakeLabeled({ "teleport", "flag_stand" }, { 0 }) };
    Block incoming{ MakeLabeled({ "flag_stand" }, { 0 }) };
    Put<std::uint32_t>(incoming, k_MapVariantMapIdOffset + 0x20, 0xCAFEBABE);
    Put<std::uint8_t>(incoming, k_PlacementsOffset + 8, 0x77);

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    Block expected{ incoming };
    // The table is rebuilt with the labels of current kept, and the placement label is the only other byte that moves.
    std::ranges::copy(remap->Block | std::views::drop(k_MapVariantLabelStringsOffset) | std::views::take(k_MapVariantLabelStringsSize),
        expected.begin() + k_MapVariantLabelStringsOffset);
    std::ranges::copy(remap->Block | std::views::drop(k_MapVariantLabelOffsetsOffset) | std::views::take(k_MaxLabels * 2),
        expected.begin() + k_MapVariantLabelOffsetsOffset);
    Put<std::int32_t>(expected, k_MapVariantLabelCountOffset, 2);
    SetLabel(expected, 0, 1);
    EXPECT_EQ(remap->Block, expected);
}

TEST(MapVariantRemapTests, MoreLabelsThanTheTableHoldsIsAnErrorAndNothingIsWritten)
{
    std::vector<std::string> many{};
    for (std::size_t index{ 0 }; index < k_MaxLabels; ++index) many.push_back(std::format("l{}", index));
    const Block current{ MakeLabeled(many, {}) };
    const Block incoming{ MakeLabeled({ "one_more" }, { 0 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_FALSE(remap.has_value());
    EXPECT_NE(remap.error().find("labels do not fit"), std::string::npos);
}

TEST(MapVariantRemapTests, MoreStringBytesThanTheAreaHoldsIsAnError)
{
    const Block current{ MakeLabeled({ std::string(1500, 'a'), std::string(1500, 'b') }, {}) };
    const Block incoming{ MakeLabeled({ std::string(1500, 'c') }, { 0 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_FALSE(remap.has_value());
    EXPECT_NE(remap.error().find("bytes do not fit"), std::string::npos);
}

TEST(MapVariantRemapTests, ATableThatIsNotValidIsAnError)
{
    Block bad{ MakeLabeled({ "a" }, {}) };
    Put<std::int32_t>(bad, k_MapVariantLabelCountOffset, 0x101);
    const Block good{ MakeLabeled({ "a" }, { 0 }) };

    EXPECT_FALSE(RemapLabels(bad, good).has_value());
    EXPECT_FALSE(RemapLabels(good, bad).has_value());

    Block unterminated{ MakeLabeled({ "a" }, {}) };
    std::memset(unterminated.data() + k_MapVariantLabelStringsOffset, 'x', k_MapVariantLabelStringsSize);
    EXPECT_FALSE(RemapLabels(good, unterminated).has_value());

    EXPECT_FALSE(RemapLabels(Block(10), good).has_value());
}

TEST(MapVariantRemapTests, TheResultStillPassesCheckCompatible)
{
    const Block current{ MakeLabeled({ "teleport", "flag_stand" }, { 0, 1 }) };
    const Block incoming{ MakeLabeled({ "flag_stand", "teleporter", "blink_spawn" }, { 0, 1, 2 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    const auto header{ Read(remap->Block) };
    ASSERT_TRUE(header.has_value());
    EXPECT_FALSE(CheckCompatible(*header, *Read(current)).has_value());
    EXPECT_EQ(header->PlacementsInUse, 3u);
    EXPECT_EQ(ComposeRuntime(remap->Block, current).size(), k_MapVariantSize);
}

TEST(MapVariantRemapTests, TwoSwitchesInARowKeepTheIndicesOfTheFirst)
{
    // The runtime after a remap is the current one of the next: First -> Open Flag -> First again gives First back its own indices.
    const Block first{ MakeLabeled({ "teleport", "flag_stand" }, { 0, 1 }) };
    const Block openFlag{ MakeLabeled({ "flag_stand", "teleporter" }, { 0, 1 }) };

    const auto toOpenFlag{ RemapLabels(first, openFlag) };
    ASSERT_TRUE(toOpenFlag.has_value());
    const auto backToFirst{ RemapLabels(toOpenFlag->Block, first) };

    ASSERT_TRUE(backToFirst.has_value());
    EXPECT_EQ(LabelOf(backToFirst->Block, 0), 0);
    EXPECT_EQ(LabelOf(backToFirst->Block, 1), 1);
    EXPECT_EQ(backToFirst->Moved, 0u);
    EXPECT_EQ(backToFirst->Appended, 0u);
}

TEST(MapVariantRemapTests, APlacementWhoseLabelIsOutsideTheIncomingTableHasNoLabelAfterTheMerge)
{
    const Block current{ MakeLabeled({ "teleport", "flag_stand" }, { 0 }) };
    // The incoming table has 1 label and the placement points at index 5: after the merge index 5 would be another label (or none).
    const Block incoming{ MakeLabeled({ "flag_stand" }, { 0, 5 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    EXPECT_EQ(LabelOf(remap->Block, 0), 1);
    EXPECT_EQ(LabelOf(remap->Block, 1), k_NoLabel);
}

TEST(MapVariantRemapTests, AThroughBThroughCBackToAKeepsTheIndicesOfAAndTheTableHoldsEveryLabel)
{
    const Block a{ MakeLabeled({ "teleport", "flag_stand" }, { 0, 1 }) };
    const Block b{ MakeLabeled({ "flag_stand", "teleporter", "blink_spawn" }, { 0, 1, 2 }) };
    const Block c{ MakeLabeled({ "blink_spawn", "bottom", "teleport" }, { 0, 1, 2 }) };

    const auto toB{ RemapLabels(a, b) };
    ASSERT_TRUE(toB.has_value());
    const auto toC{ RemapLabels(toB->Block, c) };
    ASSERT_TRUE(toC.has_value());
    const auto backToA{ RemapLabels(toC->Block, a) };
    ASSERT_TRUE(backToA.has_value());

    // teleport 0, flag_stand 1, teleporter 2, blink_spawn 3, bottom 4: the labels of A kept the index they had in A all along.
    EXPECT_EQ(*ReadLabels(backToA->Block), (std::vector<std::string>{ "teleport", "flag_stand", "teleporter", "blink_spawn", "bottom" }));
    EXPECT_EQ(LabelOf(backToA->Block, 0), 0);
    EXPECT_EQ(LabelOf(backToA->Block, 1), 1);
    EXPECT_EQ(backToA->Moved, 0u);
    EXPECT_EQ(backToA->Appended, 0u);
    EXPECT_EQ(backToA->Kept, 3u);

    EXPECT_EQ(LabelOf(toC->Block, 0), 3);
    EXPECT_EQ(LabelOf(toC->Block, 1), 4);
    EXPECT_EQ(LabelOf(toC->Block, 2), 0);
}

TEST(MapVariantRemapTests, ANegativeCountAndAnOffsetOutsideTheAreaAreRefused)
{
    const Block good{ MakeLabeled({ "a" }, { 0 }) };

    Block negative{ MakeLabeled({ "a" }, {}) };
    Put<std::int32_t>(negative, k_MapVariantLabelCountOffset, -1);
    EXPECT_FALSE(RemapLabels(negative, good).has_value());
    EXPECT_FALSE(RemapLabels(good, negative).has_value());

    Block outside{ MakeLabeled({ "a" }, {}) };
    Put<std::uint16_t>(outside, k_MapVariantLabelOffsetsOffset, static_cast<std::uint16_t>(k_MapVariantLabelStringsSize));
    EXPECT_FALSE(RemapLabels(outside, good).has_value());
    EXPECT_FALSE(RemapLabels(good, outside).has_value());
    EXPECT_FALSE(ReadLabels(outside).has_value());
}

TEST(MapVariantRemapTests, AZeroLengthLabelIsAnOrdinaryLabel)
{
    const Block current{ MakeLabeled({ "", "flag_stand" }, { 0, 1 }) };
    const Block incoming{ MakeLabeled({ "flag_stand", "" }, { 0, 1 }) };

    const auto remap{ RemapLabels(current, incoming) };

    ASSERT_TRUE(remap.has_value());
    EXPECT_EQ(*ReadLabels(remap->Block), (std::vector<std::string>{ "", "flag_stand" }));
    EXPECT_EQ(LabelOf(remap->Block, 0), 1);
    EXPECT_EQ(LabelOf(remap->Block, 1), 0);
    EXPECT_EQ(remap->Appended, 0u);
    EXPECT_EQ(remap->Moved, 2u);
}
