#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.Layout.Object.Type;
import Blam.Layout.Player.Type;
import Blam.Layout.Interaction.Type;
import Blam.Layout.Game.Type;
import Blam.Layout.Snapshot.Type;
import std;

// The Snapshot of Blam.Layout (M4a, D184-D186): reads relative to a region, the size of every region and the raw object, without MCC.

namespace
{
    using Blam::Layout::Snapshot::Type::RegionId;
    using Blam::Layout::Snapshot::Type::Region;
    using Blam::Layout::Snapshot::Type::RawObject;
    using Blam::Layout::Snapshot::Type::BlockRange;
    using Blam::Layout::Snapshot::Type::Snapshot;
    using Blam::Layout::Snapshot::Type::RegionSize;

    using Blam::Layout::Snapshot::Type::k_RegionCount;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxLocalUsers;
    using Blam::Layout::Object::Type::Constant::k_PlacementSize;
    using Blam::Layout::Object::Type::Constant::k_MaxPlacements;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsTickFractionOffset;
    using Blam::Layout::Object::Type::Constant::k_EventGlobalsSequenceOffset;
    using Blam::Layout::Object::Type::Constant::k_GameModeTypeOffset;
    using Blam::Layout::Player::Type::Size::k_Base;
    using Blam::Layout::Player::Type::Mapping::k_PlayerInputUser;
    using Blam::Layout::Player::Type::Mapping::k_OutputUserPlayer;
    using Blam::Layout::Player::Type::Mapping::k_EntryStride;
    using Blam::Layout::Interaction::Type::Offset::k_OutputUserStride;
    using Blam::Layout::Game::Type::Offset::k_GameEnginePlayerScores;
    using Blam::Layout::Game::Type::Offset::k_PlayerScoreStride;
    using Blam::Layout::Game::Type::Offset::k_EventRing;
    using Blam::Layout::Game::Type::Offset::k_EventRingSlots;
    using Blam::Layout::Game::Type::Offset::k_EventEntrySize;

    constexpr std::array<RegionId, 9> k_AllRegions{ RegionId::PlayerArray, RegionId::PlayerMapping, RegionId::Interaction,
        RegionId::TimeGlobals, RegionId::GameEngineGlobals, RegionId::EventGlobals, RegionId::Placements, RegionId::GameGlobals,
        RegionId::MegaloObjects };

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
    EXPECT_TRUE(snapshot.RawObjects.IsEmpty());
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

// Characterization (D249): each region copies the whole block the engine registers for it, or the whole array.
TEST(SnapshotTest, RegionSizesAreTheEngineAllocations)
{
    EXPECT_EQ(RegionSize(RegionId::PlayerArray), 0x4900u);
    EXPECT_EQ(RegionSize(RegionId::PlayerMapping), 0xE8u);
    EXPECT_EQ(RegionSize(RegionId::Interaction), 0x2C0u);
    EXPECT_EQ(RegionSize(RegionId::TimeGlobals), 0x3Cu);
    EXPECT_EQ(RegionSize(RegionId::GameEngineGlobals), 0x8430u);
    EXPECT_EQ(RegionSize(RegionId::EventGlobals), 0x10B68u);
    EXPECT_EQ(RegionSize(RegionId::Placements), 0x28Bu * 0x4Cu);
    EXPECT_EQ(RegionSize(RegionId::GameGlobals), 0x1F0E0u);
    EXPECT_EQ(RegionSize(RegionId::MegaloObjects), 0x200u * 0x94u);
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
    EXPECT_GT(RegionSize(RegionId::GameGlobals), k_GameModeTypeOffset);
    EXPECT_EQ(k_MaxLocalUsers, 4u);
}

TEST(RawObjectTest, ReadsInsideTheDatumOnly)
{
    std::vector<std::uint8_t> bytes(0x20, 0);
    Put<std::uint32_t>(bytes, 0x1C, 42);
    const RawObject object{ .Handle = 0xE0010005, .Address = 0x1000, .KindId = 0x0E, .Bytes = bytes };

    EXPECT_EQ(object.KindId, 0x0Eu);
    EXPECT_EQ(object.Read<std::uint32_t>(0x1C), 42u);
    EXPECT_EQ(object.Read<std::uint32_t>(0x1D), 0u);

    std::array<std::uint8_t, 4> out{};
    EXPECT_TRUE(object.ReadRaw(0x1C, out.data(), out.size()));
    EXPECT_FALSE(object.ReadRaw(0x1D, out.data(), out.size()));
}

TEST(RawObjectTest, FindBlockFollowsTheDescriptor)
{
    std::vector<std::uint8_t> bytes(0x80, 0);
    // Descriptors { u16 size, u16 offset }.
    Put<std::uint16_t>(bytes, 0x00, 0x08);
    Put<std::uint16_t>(bytes, 0x02, 0x40);
    Put<std::uint16_t>(bytes, 0x04, 0x00);
    Put<std::uint16_t>(bytes, 0x06, 0x40);
    Put<std::uint16_t>(bytes, 0x08, 0x08);
    Put<std::uint16_t>(bytes, 0x0A, 0xFFFF);
    Put<std::uint16_t>(bytes, 0x0C, 0x10);
    Put<std::uint16_t>(bytes, 0x0E, 0x78);
    Put<std::uint32_t>(bytes, 0x44, 7);

    const RawObject object{ .Bytes = bytes };
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

// --- Player traits (A19, D262) ---

namespace
{
    using Common::Semantic::Type::Subtype::Powerup;
    using Blam::Layout::Player::Type::Powerup::ActivePowerup;
    using Blam::Layout::Player::Type::Traits::PlayerTraits;
    using Blam::Layout::Player::Type::Traits::Decode;
    using Blam::Layout::Player::Type::Traits::InfiniteAmmoMode;
    using Blam::Layout::Player::Type::Traits::AbilityUsageMode;
    using Blam::Layout::Player::Type::Traits::VehicleUseMode;
    using Blam::Layout::Player::Type::Traits::IdentityVisibility;
    using Blam::Layout::Player::Type::Traits::RadarDisplayMode;
    using Blam::Layout::Player::Type::Traits::k_Size;
    using Blam::Layout::Player::Type::Traits::k_DamageResistance;
    using Blam::Layout::Player::Type::Traits::k_HealthMultiplier;
    using Blam::Layout::Player::Type::Traits::k_HealthRegenRate;
    using Blam::Layout::Player::Type::Traits::k_ShieldMultiplier;
    using Blam::Layout::Player::Type::Traits::k_ShieldRegenRate;
    using Blam::Layout::Player::Type::Traits::k_OvershieldRegenRate;
    using Blam::Layout::Player::Type::Traits::k_ShieldVampirism;
    using Blam::Layout::Player::Type::Traits::k_DamageMultiplier;
    using Blam::Layout::Player::Type::Traits::k_MeleeMultiplier;
    using Blam::Layout::Player::Type::Traits::k_MovementSpeed;
    using Blam::Layout::Player::Type::Traits::k_Gravity;
    using Blam::Layout::Player::Type::Traits::k_JumpHeight;
    using Blam::Layout::Player::Type::Traits::k_RadarRange;
    using Blam::Layout::Player::Type::Traits::k_HeadshotImmune;
    using Blam::Layout::Player::Type::Traits::k_AssassinationImmune;
    using Blam::Layout::Player::Type::Traits::k_CannotDieFromDamage;
    using Blam::Layout::Player::Type::Traits::k_GrenadeRegen;
    using Blam::Layout::Player::Type::Traits::k_WeaponPickup;
    using Blam::Layout::Player::Type::Traits::k_AbilitiesDropOnDeath;
    using Blam::Layout::Player::Type::Traits::k_InfiniteAbilities;
    using Blam::Layout::Player::Type::Traits::k_InfiniteAmmo;
    using Blam::Layout::Player::Type::Traits::k_AbilityUsage;
    using Blam::Layout::Player::Type::Traits::k_VehicleUse;
    using Blam::Layout::Player::Type::Traits::k_VisibleWaypoint;
    using Blam::Layout::Player::Type::Traits::k_VisibleName;
    using Blam::Layout::Player::Type::Traits::k_RadarMode;
    using Blam::Layout::Player::Type::Traits::k_GrenadeCount;
    using Blam::Layout::Player::Type::Traits::k_ActiveCamo;
    using Blam::Layout::Player::Type::Traits::k_PrimaryWeapon;
    using Blam::Layout::Player::Type::Traits::k_SecondaryWeapon;
    using Blam::Layout::Player::Type::Traits::k_Ability;

    using TraitsBlock = std::array<std::uint8_t, k_Size>;

    // return: The traits of a block of zeros with one byte set.
    auto DecodeByte(std::size_t offset, std::uint8_t raw) -> PlayerTraits
    {
        TraitsBlock block{};
        block[offset] = raw;
        return Decode(block);
    }

    // Checks the value of a numeric trait at the first and last raw index, and NaN at 0 and past the last one.
    auto ExpectFloatEdges(std::size_t offset, float PlayerTraits::* field, float first, float last, std::uint8_t lastIndex) -> void
    {
        EXPECT_FLOAT_EQ(DecodeByte(offset, 1).*field, first) << offset;
        if (std::isinf(last)) EXPECT_TRUE(std::isinf(DecodeByte(offset, lastIndex).*field)) << offset;
        else EXPECT_FLOAT_EQ(DecodeByte(offset, lastIndex).*field, last) << offset;
        EXPECT_TRUE(std::isnan(DecodeByte(offset, 0).*field)) << offset;
        EXPECT_TRUE(std::isnan(DecodeByte(offset, static_cast<std::uint8_t>(lastIndex + 1)).*field)) << offset;
        EXPECT_TRUE(std::isnan(DecodeByte(offset, 255).*field)) << offset;
    }
}

TEST(PlayerTraitsTest, OffsetIsInsideThePlayerEntry)
{
    EXPECT_EQ(Blam::Layout::Player::Type::Offset::k_Traits + k_Size, 0x328u + 0x2Cu);
    EXPECT_LE(Blam::Layout::Player::Type::Offset::k_Traits + k_Size, k_Base);
}

TEST(PlayerTraitsTest, NumericTablesDecodeTheirEdgesAndNaNOutside)
{
    ExpectFloatEdges(k_DamageResistance, &PlayerTraits::DamageResistance, 0.1f, std::numeric_limits<float>::infinity(), 12);
    ExpectFloatEdges(k_HealthMultiplier, &PlayerTraits::HealthMultiplier, 0.0f, 4.0f, 6);
    ExpectFloatEdges(k_HealthRegenRate, &PlayerTraits::HealthRegenRate, -0.25f, 2.0f, 9);
    ExpectFloatEdges(k_ShieldMultiplier, &PlayerTraits::ShieldMultiplier, 0.0f, 4.0f, 6);
    ExpectFloatEdges(k_ShieldRegenRate, &PlayerTraits::ShieldRegenRate, -0.25f, 2.0f, 14);
    ExpectFloatEdges(k_OvershieldRegenRate, &PlayerTraits::OvershieldRegenRate, -0.25f, 2.0f, 14);
    ExpectFloatEdges(k_ShieldVampirism, &PlayerTraits::ShieldVampirism, 0.0f, 1.0f, 5);
    ExpectFloatEdges(k_DamageMultiplier, &PlayerTraits::DamageMultiplier, 0.0f, std::numeric_limits<float>::infinity(), 12);
    ExpectFloatEdges(k_MeleeMultiplier, &PlayerTraits::MeleeMultiplier, 0.0f, std::numeric_limits<float>::infinity(), 12);
    ExpectFloatEdges(k_MovementSpeed, &PlayerTraits::MovementSpeed, 0.0f, 3.0f, 17);
    ExpectFloatEdges(k_Gravity, &PlayerTraits::Gravity, 0.5f, 2.0f, 5);
    ExpectFloatEdges(k_RadarRange, &PlayerTraits::RadarRange, 10.0f, 150.0f, 7);

    EXPECT_FLOAT_EQ(DecodeByte(k_DamageResistance, 11).DamageResistance, 20.0f);
    EXPECT_FLOAT_EQ(DecodeByte(k_DamageMultiplier, 11).DamageMultiplier, 3.0f);
    EXPECT_FLOAT_EQ(DecodeByte(k_MovementSpeed, 6).MovementSpeed, 1.0f);
    EXPECT_FLOAT_EQ(DecodeByte(k_MovementSpeed, 7).MovementSpeed, 1.1f);
    EXPECT_FLOAT_EQ(DecodeByte(k_Gravity, 2).Gravity, 0.75f);
}

TEST(PlayerTraitsTest, JumpHeightIsAPercentage)
{
    TraitsBlock block{};
    block[k_JumpHeight] = 100;
    EXPECT_FLOAT_EQ(Decode(block).JumpHeight, 1.0f);
    block[k_JumpHeight] = 150;
    EXPECT_FLOAT_EQ(Decode(block).JumpHeight, 1.5f);
    block[k_JumpHeight] = 0x2C;
    block[k_JumpHeight + 1] = 0x01;
    EXPECT_FLOAT_EQ(Decode(block).JumpHeight, 3.0f);
    block.fill(0);
    for (std::size_t byte{ 0 }; byte < 4; ++byte) block[k_JumpHeight + byte] = 0xFF;
    EXPECT_FLOAT_EQ(Decode(block).JumpHeight, -0.01f);
}

TEST(PlayerTraitsTest, BoolsAreTwoForYesAndFalseOtherwise)
{
    for (const std::size_t offset : { k_HeadshotImmune, k_AssassinationImmune, k_CannotDieFromDamage, k_GrenadeRegen, k_WeaponPickup,
        k_AbilitiesDropOnDeath, k_InfiniteAbilities })
    {
        const auto isSet = [offset](std::uint8_t raw)
        {
            const PlayerTraits traits{ DecodeByte(offset, raw) };
            return std::array<bool, 7>{ traits.IsHeadshotImmune, traits.IsAssassinationImmune, traits.CannotDieFromDamage, traits.IsGrenadeRegenOn,
                traits.IsWeaponPickupOn, traits.DoAbilitiesDropOnDeath, traits.AreAbilitiesInfinite };
        };

        EXPECT_EQ(std::ranges::count(isSet(2), true), 1) << offset;
        EXPECT_EQ(std::ranges::count(isSet(1), true), 0) << offset;
        EXPECT_EQ(std::ranges::count(isSet(0), true), 0) << offset;
        EXPECT_EQ(std::ranges::count(isSet(3), true), 0) << offset;
    }

    EXPECT_TRUE(DecodeByte(k_HeadshotImmune, 2).IsHeadshotImmune);
    EXPECT_TRUE(DecodeByte(k_InfiniteAbilities, 2).AreAbilitiesInfinite);
}

TEST(PlayerTraitsTest, EnumsAreUnknownOutsideTheirValues)
{
    EXPECT_EQ(DecodeByte(k_InfiniteAmmo, 1).InfiniteAmmo, InfiniteAmmoMode::Off);
    EXPECT_EQ(DecodeByte(k_InfiniteAmmo, 3).InfiniteAmmo, InfiniteAmmoMode::Bottomless);
    EXPECT_EQ(DecodeByte(k_InfiniteAmmo, 0).InfiniteAmmo, InfiniteAmmoMode::Unknown);
    EXPECT_EQ(DecodeByte(k_InfiniteAmmo, 4).InfiniteAmmo, InfiniteAmmoMode::Unknown);

    EXPECT_EQ(DecodeByte(k_AbilityUsage, 1).AbilityUsage, AbilityUsageMode::Disabled);
    EXPECT_EQ(DecodeByte(k_AbilityUsage, 3).AbilityUsage, AbilityUsageMode::Enabled);
    EXPECT_EQ(DecodeByte(k_AbilityUsage, 4).AbilityUsage, AbilityUsageMode::Unknown);

    EXPECT_EQ(DecodeByte(k_VehicleUse, 1).VehicleUse, VehicleUseMode::Blocked);
    EXPECT_EQ(DecodeByte(k_VehicleUse, 6).VehicleUse, VehicleUseMode::NoDriver);
    EXPECT_EQ(DecodeByte(k_VehicleUse, 8).VehicleUse, VehicleUseMode::Full);
    EXPECT_EQ(DecodeByte(k_VehicleUse, 9).VehicleUse, VehicleUseMode::Unknown);
    EXPECT_EQ(DecodeByte(k_VehicleUse, 0).VehicleUse, VehicleUseMode::Unknown);

    EXPECT_EQ(DecodeByte(k_VisibleWaypoint, 2).VisibleWaypoint, IdentityVisibility::Allies);
    EXPECT_EQ(DecodeByte(k_VisibleWaypoint, 3).VisibleWaypoint, IdentityVisibility::Everyone);
    EXPECT_EQ(DecodeByte(k_VisibleWaypoint, 4).VisibleWaypoint, IdentityVisibility::Unknown);
    EXPECT_EQ(DecodeByte(k_VisibleName, 1).VisibleName, IdentityVisibility::Hidden);
    EXPECT_EQ(DecodeByte(k_VisibleName, 0).VisibleName, IdentityVisibility::Unknown);

    EXPECT_EQ(DecodeByte(k_RadarMode, 1).RadarMode, RadarDisplayMode::Off);
    EXPECT_EQ(DecodeByte(k_RadarMode, 4).RadarMode, RadarDisplayMode::Enhanced);
    EXPECT_EQ(DecodeByte(k_RadarMode, 5).RadarMode, RadarDisplayMode::Unknown);
    EXPECT_EQ(DecodeByte(k_RadarMode, 255).RadarMode, RadarDisplayMode::Unknown);
}

TEST(PlayerTraitsTest, GrenadeSettingGivesTheCountsOfEachKind)
{
    const std::array<std::array<std::uint8_t, 3>, 15> expected{ {
        { 0, 0, 0 }, { 1, 255, 255 }, { 2, 0, 0 }, { 3, 1, 0 }, { 4, 2, 0 }, { 5, 3, 0 }, { 6, 4, 0 }, { 7, 0, 1 }, { 8, 0, 2 }, { 9, 0, 3 },
        { 10, 0, 4 }, { 11, 1, 1 }, { 12, 2, 2 }, { 13, 3, 3 }, { 14, 4, 4 } } };
    for (const auto& row : expected)
    {
        const PlayerTraits traits{ DecodeByte(k_GrenadeCount, row[0]) };
        EXPECT_EQ(traits.FragGrenades, row[1]) << static_cast<int>(row[0]);
        EXPECT_EQ(traits.PlasmaGrenades, row[2]) << static_cast<int>(row[0]);
    }

    EXPECT_EQ(DecodeByte(k_GrenadeCount, 15).FragGrenades, 0);
    EXPECT_EQ(DecodeByte(k_GrenadeCount, 255).PlasmaGrenades, 0);
}

TEST(PlayerTraitsTest, RawFieldsKeepTheirValue)
{
    EXPECT_EQ(DecodeByte(k_ActiveCamo, 5).ActiveCamo, 5);
    EXPECT_EQ(DecodeByte(k_PrimaryWeapon, 0).PrimaryWeapon, 0);
    EXPECT_EQ(DecodeByte(k_PrimaryWeapon, 0xFF).PrimaryWeapon, -1);
    EXPECT_EQ(DecodeByte(k_SecondaryWeapon, 1).SecondaryWeapon, 1);
    EXPECT_EQ(DecodeByte(k_Ability, 0xFF).Ability, -1);
}

TEST(PlayerTraitsTest, OfAnAllZeroBlockAreUnknown)
{
    const PlayerTraits traits{ Decode(TraitsBlock{}) };
    EXPECT_TRUE(std::isnan(traits.MovementSpeed));
    EXPECT_TRUE(std::isnan(traits.RadarRange));
    EXPECT_FALSE(traits.IsHeadshotImmune);
    EXPECT_EQ(traits.InfiniteAmmo, InfiniteAmmoMode::Unknown);
    EXPECT_EQ(traits.VehicleUse, VehicleUseMode::Unknown);
}

TEST(PlayerPowerupTest, WithoutPickupsThereIsNoPowerup)
{
    const auto none{ ActivePowerup({ 0, 0, 0 }, 1000, { 600, 900, 1200 }) };
    EXPECT_EQ(none.Subtype, Powerup::None);
    EXPECT_EQ(none.Ticks, 0u);
}

TEST(PlayerPowerupTest, TheRedPowerupCountsDownFromItsDurationPlusOneToOne)
{
    const std::array<std::uint32_t, 3> durations{ 600, 900, 1200 };
    const std::array<std::int32_t, 3> pickups{ 1321, 0, 0 };
    const auto first{ ActivePowerup(pickups, 1322, durations) };
    EXPECT_EQ(first.Subtype, Powerup::Red);
    EXPECT_EQ(first.Ticks, 600u);
    const auto last{ ActivePowerup(pickups, 1921, durations) };
    EXPECT_EQ(last.Subtype, Powerup::Red);
    EXPECT_EQ(last.Ticks, 1u);
    const auto gone{ ActivePowerup(pickups, 1922, durations) };
    EXPECT_EQ(gone.Subtype, Powerup::None);
    EXPECT_EQ(gone.Ticks, 0u);
}

TEST(PlayerPowerupTest, OnlyTheMostRecentPickupCountsAndTheOlderOneDoesNotComeBack)
{
    const std::array<std::uint32_t, 3> durations{ 900, 900, 1200 };
    const std::array<std::int32_t, 3> pickups{ 0, 4015, 4253 };
    const auto yellow{ ActivePowerup(pickups, 4254, durations) };
    EXPECT_EQ(yellow.Subtype, Powerup::Yellow);
    EXPECT_EQ(yellow.Ticks, 1200u);
    EXPECT_EQ(ActivePowerup(pickups, 5453, durations).Ticks, 1u);
    const auto after{ ActivePowerup(pickups, 5454, durations) };
    EXPECT_EQ(after.Subtype, Powerup::None);
    EXPECT_EQ(after.Ticks, 0u);
}

TEST(PlayerPowerupTest, ATieTakesTheLowestIndex)
{
    const auto tie{ ActivePowerup({ 0, 50, 50 }, 51, { 600, 900, 1200 }) };
    EXPECT_EQ(tie.Subtype, Powerup::Blue);
    EXPECT_EQ(tie.Ticks, 900u);
    EXPECT_EQ(ActivePowerup({ 70, 70, 70 }, 71, { 600, 900, 1200 }).Subtype, Powerup::Red);
}

TEST(PlayerPowerupTest, ADurationOfZeroIsNeverActiveAndTheTickOfThePickupIsDefined)
{
    EXPECT_EQ(ActivePowerup({ 100, 0, 0 }, 100, { 0, 0, 0 }).Subtype, Powerup::None);
    EXPECT_EQ(ActivePowerup({ 100, 0, 0 }, 101, { 0, 900, 1200 }).Subtype, Powerup::None);
    const auto same{ ActivePowerup({ 100, 0, 0 }, 100, { 600, 900, 1200 }) };
    EXPECT_EQ(same.Subtype, Powerup::Red);
    EXPECT_EQ(same.Ticks, 601u);
}