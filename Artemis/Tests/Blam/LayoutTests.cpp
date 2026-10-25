#include <gtest/gtest.h>

import Common.Engine.Type;
import Datum.Object.Type;
import Layout.Player.Type;
import Layout.Interaction.Type;
import Layout.Game.Type;
import Layout.Snapshot.Type;
import std;

// The Snapshot of Blam.Layout (M4a, D184-D186): reads relative to a region, the size of every region and the raw object, without MCC.

namespace
{
    using Layout::Snapshot::Type::RegionId;
    using Layout::Snapshot::Type::Region;
    using Layout::Snapshot::Type::RawObject;
    using Layout::Snapshot::Type::BlockRange;
    using Layout::Snapshot::Type::Snapshot;
    using Layout::Snapshot::Type::RegionSize;

    using Layout::Snapshot::Type::k_RegionCount;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Datum::Object::Type::Constant::k_PlacementSize;
    using Datum::Object::Type::Constant::k_MaxPlacements;
    using Datum::Object::Type::Constant::k_TimeGlobalsTickFractionOffset;
    using Datum::Object::Type::Constant::k_EventGlobalsSequenceOffset;
    using Layout::Player::Type::Size::k_Base;
    using Layout::Player::Type::Mapping::k_PlayerInputUser;
    using Layout::Player::Type::Mapping::k_OutputUserPlayer;
    using Layout::Player::Type::Mapping::k_EntryStride;
    using Layout::Interaction::Type::Offset::k_OutputUserStride;
    using Layout::Game::Type::Offset::k_GameEnginePlayerScores;
    using Layout::Game::Type::Offset::k_PlayerScoreStride;
    using Layout::Game::Type::Offset::k_EventRing;
    using Layout::Game::Type::Offset::k_EventRingSlots;
    using Layout::Game::Type::Offset::k_EventEntrySize;

    constexpr std::array<RegionId, 7> k_AllRegions{ RegionId::PlayerArray, RegionId::PlayerMapping, RegionId::Interaction,
        RegionId::TimeGlobals, RegionId::GameEngineGlobals, RegionId::EventGlobals, RegionId::Placements };

    auto RegionOf(Snapshot& snapshot, RegionId id) -> Region&
    {
        return snapshot.Regions[static_cast<std::size_t>(id)];
    }

    template <typename T>
    auto Put(std::vector<std::uint8_t>& bytes, std::size_t offset, T value) -> void
    {
        std::memcpy(bytes.data() + offset, &value, sizeof(T));
    }

    // A captured region of the given size, zero filled, at a made up address.
    auto Capture(Snapshot& snapshot, RegionId id, std::size_t size) -> std::vector<std::uint8_t>&
    {
        Region& region{ RegionOf(snapshot, id) };
        region.OriginalAddress = 0x7FF000000000;
        region.Bytes.assign(size, 0);
        return region.Bytes;
    }
}

TEST(SnapshotTest, EveryRegionHasAnId)
{
    EXPECT_EQ(k_RegionCount, k_AllRegions.size());
    for (std::size_t i = 0; i < k_AllRegions.size(); ++i)
    {
        EXPECT_EQ(static_cast<std::size_t>(k_AllRegions[i]), i);
    }
}

TEST(SnapshotTest, AnEmptySnapshotHasNothing)
{
    const Snapshot snapshot{};
    EXPECT_FALSE(snapshot.HasThreadContext);
    EXPECT_FALSE(snapshot.HasObjectTable);
    EXPECT_TRUE(snapshot.RawObjects.empty());
    for (RegionId id : k_AllRegions)
    {
        EXPECT_FALSE(snapshot.IsCaptured(id));
        EXPECT_EQ(snapshot.Read<std::uint64_t>(id, 0), 0u);
    }
}

TEST(SnapshotTest, ReadsInsideTheRegion)
{
    Snapshot snapshot{};
    auto& bytes{ Capture(snapshot, RegionId::PlayerArray, 0x40) };
    Put<std::uint32_t>(bytes, 0x10, 0xDEADBEEF);
    Put<float>(bytes, 0x20, 1.5f);

    EXPECT_TRUE(snapshot.IsCaptured(RegionId::PlayerArray));
    EXPECT_EQ(snapshot.Read<std::uint32_t>(RegionId::PlayerArray, 0x10), 0xDEADBEEFu);
    EXPECT_EQ(snapshot.Read<float>(RegionId::PlayerArray, 0x20), 1.5f);
    EXPECT_EQ(snapshot.Read<std::uint16_t>(RegionId::PlayerArray, 0x12), 0xDEADu);
}

TEST(SnapshotTest, AValueThatDoesNotFitReadsZero)
{
    Snapshot snapshot{};
    auto& bytes{ Capture(snapshot, RegionId::TimeGlobals, 0x20) };
    std::ranges::fill(bytes, std::uint8_t{ 0xAB });

    EXPECT_EQ(snapshot.Read<std::uint32_t>(RegionId::TimeGlobals, 0x1C), 0xABABABABu);
    EXPECT_EQ(snapshot.Read<std::uint32_t>(RegionId::TimeGlobals, 0x1D), 0u);
    EXPECT_EQ(snapshot.Read<std::uint8_t>(RegionId::TimeGlobals, 0x1F), 0xABu);
    EXPECT_EQ(snapshot.Read<std::uint8_t>(RegionId::TimeGlobals, 0x20), 0u);
}

TEST(SnapshotTest, AHugeOffsetDoesNotWrapAround)
{
    Snapshot snapshot{};
    auto& bytes{ Capture(snapshot, RegionId::EventGlobals, 0x40) };
    std::ranges::fill(bytes, std::uint8_t{ 0xCD });

    constexpr std::size_t k_Max{ std::numeric_limits<std::size_t>::max() };
    EXPECT_EQ(snapshot.Read<std::uint32_t>(RegionId::EventGlobals, k_Max), 0u);
    EXPECT_EQ(snapshot.Read<std::uint32_t>(RegionId::EventGlobals, k_Max - 2), 0u);
    const auto array{ snapshot.ReadArray<std::uint8_t, 4>(RegionId::EventGlobals, k_Max - 1) };
    EXPECT_EQ(array, (std::array<std::uint8_t, 4>{}));
}

TEST(SnapshotTest, ReadArrayInsideAndOutside)
{
    Snapshot snapshot{};
    auto& bytes{ Capture(snapshot, RegionId::PlayerArray, 0x20) };
    const std::array<wchar_t, 4> tag{ L'A', L'R', L'T', L'M' };
    std::memcpy(bytes.data() + 0x8, tag.data(), sizeof(tag));

    EXPECT_EQ((snapshot.ReadArray<wchar_t, 4>(RegionId::PlayerArray, 0x8)), tag);
    EXPECT_EQ((snapshot.ReadArray<wchar_t, 4>(RegionId::PlayerArray, 0x1A)), (std::array<wchar_t, 4>{}));
    EXPECT_EQ((snapshot.ReadArray<wchar_t, 4>(RegionId::Interaction, 0x8)), (std::array<wchar_t, 4>{}));
}

TEST(SnapshotTest, RegionsDoNotSeeEachOther)
{
    Snapshot snapshot{};
    auto& time{ Capture(snapshot, RegionId::TimeGlobals, 0x18) };
    Put<std::uint32_t>(time, 0x0C, 1234);

    EXPECT_EQ(snapshot.Read<std::uint32_t>(RegionId::TimeGlobals, 0x0C), 1234u);
    EXPECT_FALSE(snapshot.IsCaptured(RegionId::GameEngineGlobals));
    EXPECT_EQ(snapshot.Read<std::uint32_t>(RegionId::GameEngineGlobals, 0x0C), 0u);
}

TEST(SnapshotTest, ReadPlacementKeepsTheRulesOfBefore)
{
    Snapshot snapshot{};
    EXPECT_FALSE(snapshot.ReadPlacement<std::uint16_t>(0, 0).has_value());

    auto& bytes{ Capture(snapshot, RegionId::Placements, RegionSize(RegionId::Placements)) };
    Put<std::uint16_t>(bytes, 2 * k_PlacementSize + 4, 0x1234);
    Put<std::uint8_t>(bytes, (k_MaxPlacements - 1) * k_PlacementSize, 0x77);

    EXPECT_EQ(snapshot.ReadPlacement<std::uint16_t>(2, 4), std::optional<std::uint16_t>{ 0x1234 });
    EXPECT_EQ(snapshot.ReadPlacement<std::uint8_t>(k_MaxPlacements - 1, 0), std::optional<std::uint8_t>{ 0x77 });
    EXPECT_FALSE(snapshot.ReadPlacement<std::uint16_t>(2, k_PlacementSize - 1).has_value());
    EXPECT_FALSE(snapshot.ReadPlacement<std::uint8_t>(k_MaxPlacements, 0).has_value());
}

// Characterization (D185): each region copies up to the last field the code read on 2026-10-25, or the whole known structure.
TEST(SnapshotTest, RegionSizesAreTheRangesReadToday)
{
    EXPECT_EQ(RegionSize(RegionId::PlayerArray), 0x4900u);
    EXPECT_EQ(RegionSize(RegionId::PlayerMapping), 0xC8u);
    EXPECT_EQ(RegionSize(RegionId::Interaction), 0x2C0u);
    EXPECT_EQ(RegionSize(RegionId::TimeGlobals), 0x18u);
    EXPECT_EQ(RegionSize(RegionId::GameEngineGlobals), 0x32F0u);
    EXPECT_EQ(RegionSize(RegionId::EventGlobals), 0x10B34u);
    EXPECT_EQ(RegionSize(RegionId::Placements), 0x28Bu * 0x4Cu);
}

TEST(SnapshotTest, RegionSizesCoverTheFieldsTheyHold)
{
    EXPECT_EQ(RegionSize(RegionId::PlayerArray), k_MaxPlayers * k_Base);
    EXPECT_GE(RegionSize(RegionId::PlayerMapping), k_PlayerInputUser + k_MaxPlayers * k_EntryStride);
    EXPECT_GE(RegionSize(RegionId::PlayerMapping), k_OutputUserPlayer + k_MaxLocalUsers * k_EntryStride);
    EXPECT_EQ(RegionSize(RegionId::Interaction), k_MaxLocalUsers * k_OutputUserStride);
    EXPECT_GE(RegionSize(RegionId::TimeGlobals), k_TimeGlobalsTickFractionOffset + sizeof(float));
    EXPECT_GE(RegionSize(RegionId::GameEngineGlobals), k_GameEnginePlayerScores + k_MaxPlayers * k_PlayerScoreStride);
    EXPECT_GE(RegionSize(RegionId::EventGlobals), k_EventRing + k_EventRingSlots * k_EventEntrySize);
    EXPECT_GE(RegionSize(RegionId::EventGlobals), k_EventGlobalsSequenceOffset + sizeof(std::uint32_t));
    EXPECT_EQ(RegionSize(RegionId::Placements), k_MaxPlacements * k_PlacementSize);
    EXPECT_EQ(k_MaxLocalUsers, 4u);
}

TEST(RawObjectTest, ReadsInsideTheDatumOnly)
{
    RawObject object{ .Handle = 0xE0010005, .Address = 0x1000, .KindId = 0x0E, .Bytes = std::vector<std::uint8_t>(0x20, 0) };
    Put<std::uint32_t>(object.Bytes, 0x1C, 42);

    EXPECT_EQ(object.KindId, 0x0Eu);
    EXPECT_EQ(object.Read<std::uint32_t>(0x1C), 42u);
    EXPECT_EQ(object.Read<std::uint32_t>(0x1D), 0u);

    std::array<std::uint8_t, 4> out{};
    EXPECT_TRUE(object.ReadRaw(0x1C, out.data(), out.size()));
    EXPECT_FALSE(object.ReadRaw(0x1D, out.data(), out.size()));
}

TEST(RawObjectTest, FindBlockFollowsTheDescriptor)
{
    RawObject object{ .Bytes = std::vector<std::uint8_t>(0x80, 0) };
    // Descriptors { u16 size, u16 offset }.
    Put<std::uint16_t>(object.Bytes, 0x00, 0x08);
    Put<std::uint16_t>(object.Bytes, 0x02, 0x40);
    Put<std::uint16_t>(object.Bytes, 0x04, 0x00);
    Put<std::uint16_t>(object.Bytes, 0x06, 0x40);
    Put<std::uint16_t>(object.Bytes, 0x08, 0x08);
    Put<std::uint16_t>(object.Bytes, 0x0A, 0xFFFF);
    Put<std::uint16_t>(object.Bytes, 0x0C, 0x10);
    Put<std::uint16_t>(object.Bytes, 0x0E, 0x78);
    Put<std::uint32_t>(object.Bytes, 0x44, 7);

    const std::optional<BlockRange> block{ object.FindBlock(0x00) };
    ASSERT_TRUE(block.has_value());
    EXPECT_EQ(block->Offset, 0x40u);
    EXPECT_EQ(block->Size, 0x08u);
    EXPECT_FALSE(object.FindBlock(0x04).has_value());
    EXPECT_FALSE(object.FindBlock(0x08).has_value());
    EXPECT_FALSE(object.FindBlock(0x0C).has_value());

    EXPECT_EQ(object.ReadBlock<std::uint32_t>(0x00, 4), 7u);
    EXPECT_EQ(object.ReadBlock<std::uint32_t>(0x00, 5), 0u);
    EXPECT_EQ(object.ReadBlock<std::uint32_t>(0x04, 0), 0u);
}