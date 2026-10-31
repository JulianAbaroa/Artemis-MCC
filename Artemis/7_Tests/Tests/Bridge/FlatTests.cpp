#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Contract.Bridge.Type;
import Bridge.Codec.Flat.System;
import Bridge.Codec.Tick.Type;
import Perception.Details.Type;
import Perception.Events.Type;
import Perception.Match.Type;
import std;

// The flags, the stands, the round and the team scores in the BridgeTick (B4h H.4), and the two sequences of an event.

namespace
{
    using Bridge::Codec::Flat::System::FlatService;
    using Bridge::Codec::Tick::Type::Tick;
    using Common::Semantic::Type::ChargeState;
    using Common::Semantic::Type::Event;
    using Common::Semantic::Type::FlagState;
    using Common::Semantic::Type::RoundState;
    using Common::Team::Type::Team;
    using Contract::Bridge::Type::BridgeTick;
    using Contract::Bridge::Type::BridgeTickFlag;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::ProjectileDetail;
    using Perception::Details::Type::ProjectileDetails;
    using Perception::Details::Type::StandDetail;
    using Perception::Details::Type::WeaponDetail;
    using Perception::Details::Type::WeaponDetails;
    using Perception::Events::Type::EventLog;
    using Perception::Events::Type::GameEvent;
    using Perception::Match::Type::MatchStatus;

    using Contract::Bridge::Type::k_BridgeMaxObjectives;
    using Contract::Bridge::Type::k_BridgeMaxProjectiles;
    using Contract::Bridge::Type::k_BridgeMaxWeapons;
    using Common::Engine::Type::k_NoHandle;

    auto Flat(const Tick& tick) -> std::unique_ptr<BridgeTick>
    {
        auto flat{ std::make_unique<BridgeTick>() };
        FlatService{}.Flatten(tick, *flat);
        return flat;
    }

    auto Flag(std::uint32_t handle, Team baseTeam) -> ObjectiveDetail
    {
        return ObjectiveDetail{ .Handle = handle, .BaseTeam = baseTeam, .OwnerTeam = baseTeam };
    }

    auto WithObjectives(std::shared_ptr<const ObjectiveDetails> objectives) -> Tick
    {
        Tick tick{};
        tick.ObjectiveDetails = std::move(objectives);
        return tick;
    }
}

TEST(FlatTests, FlagFieldsAreCopiedToTheObjectiveRow)
{
    ObjectiveDetails objectives{};
    objectives.Flags[0xE5A10010] = ObjectiveDetail{ .Handle = 0xE5A10010, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red,
        .State = FlagState::Dropped, .CarrierPlayer = 0xE5A20003, .CarrierBiped = 0xE5A30004, .HomeStand = 0xE5A40005, .ResetTicks = 321,
        .IsResetRunning = true, .AlliesInZone = 2 };

    const auto flat{ Flat(WithObjectives(std::make_shared<const ObjectiveDetails>(std::move(objectives)))) };

    ASSERT_EQ(flat->Header.ObjectiveCount, 1u);
    EXPECT_EQ(flat->Header.StandCount, 0u);
    const auto& row{ flat->Objectives[0] };
    EXPECT_EQ(row.Handle, 0xE5A10010u);
    EXPECT_EQ(row.BaseTeam, static_cast<std::uint8_t>(Team::Blue));
    EXPECT_EQ(row.OwnerTeam, static_cast<std::uint8_t>(Team::Red));
    EXPECT_EQ(row.State, static_cast<std::uint8_t>(FlagState::Dropped));
    EXPECT_EQ(row.CarrierPlayer, 0xE5A20003u);
    EXPECT_EQ(row.CarrierBiped, 0xE5A30004u);
    EXPECT_EQ(row.HomeStand, 0xE5A40005u);
    EXPECT_EQ(row.ResetTicks, 321u);
    EXPECT_EQ(row.IsResetRunning, 1u);
    EXPECT_EQ(row.AlliesInZone, 2u);
    EXPECT_EQ(flat->Header.Flags, 0u);
}

TEST(FlatTests, FlagsAndStandsAreOrderedByBaseTeamThenHandle)
{
    // An unordered map gives no order: the rows must not depend on it.
    ObjectiveDetails objectives{};
    objectives.Flags[0x30] = Flag(0x30, Team::Blue);
    objectives.Flags[0x20] = Flag(0x20, Team::Red);
    objectives.Flags[0x10] = Flag(0x10, Team::Blue);
    objectives.Stands[0x99] = StandDetail{ .Handle = 0x99, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red };
    objectives.Stands[0x88] = StandDetail{ .Handle = 0x88, .BaseTeam = Team::Red, .OwnerTeam = Team::Blue };

    const auto flat{ Flat(WithObjectives(std::make_shared<const ObjectiveDetails>(std::move(objectives)))) };

    ASSERT_EQ(flat->Header.ObjectiveCount, 3u);
    EXPECT_EQ(flat->Objectives[0].Handle, 0x20u);
    EXPECT_EQ(flat->Objectives[1].Handle, 0x10u);
    EXPECT_EQ(flat->Objectives[2].Handle, 0x30u);

    ASSERT_EQ(flat->Header.StandCount, 2u);
    EXPECT_EQ(flat->Stands[0].Handle, 0x88u);
    EXPECT_EQ(flat->Stands[0].BaseTeam, static_cast<std::uint8_t>(Team::Red));
    EXPECT_EQ(flat->Stands[0].OwnerTeam, static_cast<std::uint8_t>(Team::Blue));
    EXPECT_EQ(flat->Stands[1].Handle, 0x99u);
}

TEST(FlatTests, MoreFlagsThanTheCapacityAreCutAndFlagged)
{
    ObjectiveDetails objectives{};
    for (std::uint32_t i{ 0 }; i < k_BridgeMaxObjectives + 1; ++i) objectives.Flags[0x100 + i] = Flag(0x100 + i, Team::Red);

    const auto flat{ Flat(WithObjectives(std::make_shared<const ObjectiveDetails>(std::move(objectives)))) };

    EXPECT_EQ(flat->Header.ObjectiveCount, k_BridgeMaxObjectives);
    EXPECT_NE(flat->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::ObjectivesTruncated), 0u);
}

TEST(FlatTests, WithoutObjectiveDetailsTheCountsAreZero)
{
    const auto flat{ Flat(Tick{}) };

    EXPECT_EQ(flat->Header.ObjectiveCount, 0u);
    EXPECT_EQ(flat->Header.StandCount, 0u);
    EXPECT_EQ(flat->Header.Flags, 0u);
}

TEST(FlatTests, WeaponFieldsAreCopiedToTheWeaponRow)
{
    WeaponDetails weapons{};
    WeaponDetail detail{ .Handle = 0xE4370001, .MagazineCount = 1, .IsEnergyWeapon = false, .Energy = std::numeric_limits<float>::quiet_NaN(),
        .Heat = 0.25f, .IsOverheated = true, .IsReloading = true, .ChargeState = ChargeState::Charging, .ChargeFraction = 0.5f,
        .TrackedObject = 0xE4280002, .IsLocked = true, .CanFireNow = false };
    detail.Magazines[0] = { .Loaded = 12, .Reserve = 48 };
    detail.Magazines[1] = { .Loaded = 3, .Reserve = 9 };
    weapons[detail.Handle] = detail;

    Tick tick{};
    tick.WeaponDetails = std::make_shared<const WeaponDetails>(std::move(weapons));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.WeaponCount, 1u);
    EXPECT_EQ(flat->Header.ProjectileCount, 0u);
    const auto& row{ flat->Weapons[0] };
    EXPECT_EQ(row.Handle, 0xE4370001u);
    EXPECT_EQ(row.MagazineCount, 1u);
    EXPECT_EQ(row.Magazines[0].Loaded, 12u);
    EXPECT_EQ(row.Magazines[0].Reserve, 48u);
    EXPECT_EQ(row.Magazines[1].Loaded, 3u);
    EXPECT_EQ(row.Magazines[1].Reserve, 9u);
    EXPECT_EQ(row.IsEnergyWeapon, 0u);
    EXPECT_TRUE(std::isnan(row.Energy));
    EXPECT_FLOAT_EQ(row.Heat, 0.25f);
    EXPECT_EQ(row.IsOverheated, 1u);
    EXPECT_EQ(row.IsReloading, 1u);
    EXPECT_EQ(row.ChargeState, static_cast<std::uint8_t>(ChargeState::Charging));
    EXPECT_FLOAT_EQ(row.ChargeFraction, 0.5f);
    EXPECT_EQ(row.TrackedObject, 0xE4280002u);
    EXPECT_EQ(row.IsLocked, 1u);
    EXPECT_EQ(row.CanFireNow, 0u);
    EXPECT_EQ(flat->Header.Flags, 0u);
}

TEST(FlatTests, ProjectileFieldsAreCopiedToTheProjectileRow)
{
    ProjectileDetails projectiles{};
    projectiles[0xE4E30001] = ProjectileDetail{ .Handle = 0xE4E30001, .OwnerPlayer = 0xEC700000, .OwnerBiped = 0xE43301C4,
        .ShooterRoot = 0xE4440005, .StuckTo = 0xE4D50006, .StuckNode = 7, .InHand = false };
    projectiles[0xE4E30002] = ProjectileDetail{ .Handle = 0xE4E30002, .OwnerPlayer = 0xEC700000, .OwnerBiped = 0xE43301C4,
        .ShooterRoot = 0xE43301C4, .StuckTo = k_NoHandle, .StuckNode = -1, .InHand = true };

    Tick tick{};
    tick.ProjectileDetails = std::make_shared<const ProjectileDetails>(std::move(projectiles));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.ProjectileCount, 2u);
    EXPECT_EQ(flat->Header.WeaponCount, 0u);
    const auto& stuck{ flat->Projectiles[0] };
    EXPECT_EQ(stuck.Handle, 0xE4E30001u);
    EXPECT_EQ(stuck.OwnerPlayer, 0xEC700000u);
    EXPECT_EQ(stuck.OwnerBiped, 0xE43301C4u);
    EXPECT_EQ(stuck.ShooterRoot, 0xE4440005u);
    EXPECT_EQ(stuck.StuckTo, 0xE4D50006u);
    EXPECT_EQ(stuck.StuckNode, 7);
    EXPECT_EQ(stuck.InHand, 0u);
    const auto& inHand{ flat->Projectiles[1] };
    EXPECT_EQ(inHand.StuckTo, k_NoHandle);
    EXPECT_EQ(inHand.StuckNode, -1);
    EXPECT_EQ(inHand.InHand, 1u);
}

TEST(FlatTests, WeaponsAndProjectilesAreOrderedByHandle)
{
    // An unordered map gives no order: the rows must not depend on it.
    WeaponDetails weapons{};
    for (const std::uint32_t handle : { 0x30u, 0x10u, 0x20u }) weapons[handle] = WeaponDetail{ .Handle = handle };
    ProjectileDetails projectiles{};
    for (const std::uint32_t handle : { 0x90u, 0x70u, 0x80u }) projectiles[handle] = ProjectileDetail{ .Handle = handle };

    Tick tick{};
    tick.WeaponDetails = std::make_shared<const WeaponDetails>(std::move(weapons));
    tick.ProjectileDetails = std::make_shared<const ProjectileDetails>(std::move(projectiles));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.WeaponCount, 3u);
    EXPECT_EQ(flat->Weapons[0].Handle, 0x10u);
    EXPECT_EQ(flat->Weapons[1].Handle, 0x20u);
    EXPECT_EQ(flat->Weapons[2].Handle, 0x30u);
    ASSERT_EQ(flat->Header.ProjectileCount, 3u);
    EXPECT_EQ(flat->Projectiles[0].Handle, 0x70u);
    EXPECT_EQ(flat->Projectiles[1].Handle, 0x80u);
    EXPECT_EQ(flat->Projectiles[2].Handle, 0x90u);
}

TEST(FlatTests, MoreWeaponsOrProjectilesThanTheCapacityAreCutAndFlagged)
{
    WeaponDetails weapons{};
    for (std::uint32_t i{ 0 }; i < k_BridgeMaxWeapons + 1; ++i) weapons[0x100 + i] = WeaponDetail{ .Handle = 0x100 + i };

    Tick onlyWeapons{};
    onlyWeapons.WeaponDetails = std::make_shared<const WeaponDetails>(std::move(weapons));
    const auto flatWeapons{ Flat(onlyWeapons) };
    EXPECT_EQ(flatWeapons->Header.WeaponCount, k_BridgeMaxWeapons);
    EXPECT_NE(flatWeapons->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::WeaponsTruncated), 0u);
    EXPECT_EQ(flatWeapons->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::ProjectilesTruncated), 0u);
    // The cut keeps the lowest handles.
    EXPECT_EQ(flatWeapons->Weapons[k_BridgeMaxWeapons - 1].Handle, 0x100u + k_BridgeMaxWeapons - 1);

    ProjectileDetails projectiles{};
    for (std::uint32_t i{ 0 }; i < k_BridgeMaxProjectiles + 1; ++i) projectiles[0x100 + i] = ProjectileDetail{ .Handle = 0x100 + i };

    Tick onlyProjectiles{};
    onlyProjectiles.ProjectileDetails = std::make_shared<const ProjectileDetails>(std::move(projectiles));
    const auto flatProjectiles{ Flat(onlyProjectiles) };
    EXPECT_EQ(flatProjectiles->Header.ProjectileCount, k_BridgeMaxProjectiles);
    EXPECT_NE(flatProjectiles->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::ProjectilesTruncated), 0u);
    EXPECT_EQ(flatProjectiles->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::WeaponsTruncated), 0u);
}

TEST(FlatTests, WithoutWeaponOrProjectileDetailsTheCountsAreZero)
{
    const auto flat{ Flat(Tick{}) };

    EXPECT_EQ(flat->Header.WeaponCount, 0u);
    EXPECT_EQ(flat->Header.ProjectileCount, 0u);
}

TEST(FlatTests, MatchCarriesTheRoundAndTheScoresOfEachTeam)
{
    MatchStatus match{};
    match.GameTick = 4000;
    match.RoundIndex = 2;
    match.RoundState = RoundState::Between;
    match.TeamScores[0] = { .Round = 1, .Total = 3 };
    match.TeamScores[1] = { .Round = 0, .Total = 2 };
    match.TeamScores[7] = { .Round = -1, .Total = 9 };
    match.RoundClockTicks = 7200;
    match.RoundTicksRemaining = -1;
    match.RoundTimeLimitTicks = 36000;
    match.RoundLimit = 3;
    match.ScoreToWin = 5;

    Tick tick{};
    tick.Match = std::make_shared<const MatchStatus>(std::move(match));
    const auto flat{ Flat(tick) };

    EXPECT_EQ(flat->Match.GameTick, 4000u);
    EXPECT_EQ(flat->Match.RoundIndex, 2);
    EXPECT_EQ(flat->Match.RoundState, static_cast<std::uint8_t>(RoundState::Between));
    EXPECT_EQ(flat->Match.TeamRoundScores[0], 1);
    EXPECT_EQ(flat->Match.TeamTotalScores[0], 3);
    EXPECT_EQ(flat->Match.TeamRoundScores[1], 0);
    EXPECT_EQ(flat->Match.TeamTotalScores[1], 2);
    EXPECT_EQ(flat->Match.TeamRoundScores[7], -1);
    EXPECT_EQ(flat->Match.TeamTotalScores[7], 9);
    EXPECT_EQ(flat->Match.TeamRoundScores[2], 0);
    EXPECT_EQ(flat->Match.RoundClockTicks, 7200u);
    EXPECT_EQ(flat->Match.RoundTicksRemaining, -1);
    EXPECT_EQ(flat->Match.RoundTimeLimitTicks, 36000u);
    EXPECT_EQ(flat->Match.RoundLimit, 3);
    EXPECT_EQ(flat->Match.ScoreToWin, 5);
}

TEST(FlatTests, HeaderCarriesTheMapId)
{
    Tick tick{};
    tick.MapId = 1234;
    EXPECT_EQ(Flat(tick)->Header.MapId, 1234);
    EXPECT_EQ(Flat(Tick{})->Header.MapId, 0);
}

TEST(FlatTests, EventsCarryTheSequenceOfArtemisAndTheOneOfTheEngine)
{
    EventLog log{};
    log.Rows.push_back(GameEvent{ .Sequence = 1, .EngineSequence = 57, .GameTick = 10, .Kind = Event::FlagTaken });
    log.Rows.push_back(GameEvent{ .Sequence = 2, .EngineSequence = 0, .GameTick = 11, .Kind = Event::FlagDropped });
    log.LastSequence = 2;

    Tick tick{};
    tick.Events = std::make_shared<const EventLog>(std::move(log));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.EventCount, 2u);
    EXPECT_EQ(flat->Events[0].Sequence, 1u);
    EXPECT_EQ(flat->Events[0].EngineSequence, 57u);
    EXPECT_EQ(flat->Events[1].Sequence, 2u);
    EXPECT_EQ(flat->Events[1].EngineSequence, 0u);
    EXPECT_EQ(flat->Header.LastEventSequence, 2u);
}