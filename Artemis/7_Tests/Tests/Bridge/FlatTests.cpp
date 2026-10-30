#include <gtest/gtest.h>

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
    using Common::Semantic::Type::Event;
    using Common::Semantic::Type::FlagState;
    using Common::Semantic::Type::RoundState;
    using Common::Team::Type::Team;
    using Contract::Bridge::Type::BridgeTick;
    using Contract::Bridge::Type::BridgeTickFlag;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::StandDetail;
    using Perception::Events::Type::EventLog;
    using Perception::Events::Type::GameEvent;
    using Perception::Match::Type::MatchStatus;

    using Contract::Bridge::Type::k_BridgeMaxObjectives;

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