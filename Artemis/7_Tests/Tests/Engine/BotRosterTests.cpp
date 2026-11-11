#include <gtest/gtest.h>

import Common.Engine.Type;
import Blam.Layout.Game.Type;
import Engine.Control.Simulation.Type;
import Engine.Control.Simulation.State;
import std;

// The bot roster (LS.1, D284): the plan from the bots of the match to a roster, the lowest free bot number, the player update buffer
// the engine function takes and the removal requests of the bot store. All of it without MCC.

namespace
{
    using Engine::Control::Simulation::State::BotStore;
    using Engine::Control::Simulation::Type::Bot;
    using Engine::Control::Simulation::Type::FindFreeBotNumber;
    using Engine::Control::Simulation::Type::MakePlayerUpdate;
    using Engine::Control::Simulation::Type::GetRosterStep;
    using Engine::Control::Simulation::Type::PlanRoster;
    using Engine::Control::Simulation::Type::Roster;
    using Engine::Control::Simulation::Type::RosterOutcome;
    using Engine::Control::Simulation::Type::RosterResult;
    using Engine::Control::Simulation::Type::RosterStep;
    using Engine::Control::Simulation::Type::ToText;
    using Engine::Control::Simulation::Type::RosterBot;
    using Engine::Control::Simulation::Type::RosterMove;
    using Engine::Control::Simulation::Type::RosterPlan;

    using Common::Engine::Type::k_MaxTeams;
    using Engine::Control::Simulation::Type::k_MaxBots;
    using Engine::Control::Simulation::Type::k_PlayerUpdateAdded;
    using Engine::Control::Simulation::Type::k_PlayerUpdateLeft;
    using Engine::Control::Simulation::Type::k_PlayerUpdateRemove;
    using Engine::Control::Simulation::Type::k_PlayerUpdateSize;
    using Blam::Layout::Game::Type::Offset::k_RoundStateBetween;
    using Blam::Layout::Game::Type::Offset::k_RoundStateEnded;
    using Engine::Control::Simulation::Type::k_UpdateIdentifier;
    using Engine::Control::Simulation::Type::k_UpdatePlayerIndex;
    using Engine::Control::Simulation::Type::k_UpdateType;

    // return: Bots numbered from 1 in the order of the counts: the first team 0, then team 1, and so on.
    auto MakeBots(std::initializer_list<std::size_t> perTeam) -> std::vector<RosterBot>
    {
        std::vector<RosterBot> bots{};
        std::size_t number{ 1 };
        std::int8_t team{ 0 };

        for (const std::size_t count : perTeam)
        {
            for (std::size_t index{ 0 }; index < count; ++index, ++number)
            {
                bots.push_back(RosterBot{ .Number = number, .PlayerIndex = 16 - number, .Team = team });
            }

            ++team;
        }

        return bots;
    }

    auto MakeRoster(std::initializer_list<std::size_t> perTeam, std::size_t autoCount = 0) -> Roster
    {
        Roster roster{ .Auto = autoCount };
        std::size_t team{ 0 };

        for (const std::size_t count : perTeam) roster.Teams[team++] = count;

        return roster;
    }
}

TEST(BotRosterTests, SameRosterPlansNothing)
{
    const auto plan{ PlanRoster(MakeBots({ 3, 4 }), MakeRoster({ 3, 4 }), 0, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_TRUE(plan->IsEmpty());
}

TEST(BotRosterTests, EightVersusEightToTwoVersusTwoRemovesTwelveHighestFirst)
{
    const auto plan{ PlanRoster(MakeBots({ 8, 8 }), MakeRoster({ 2, 2 }), 0, 0) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_TRUE(plan->Moves.empty());
    EXPECT_TRUE(plan->Adds.empty());
    EXPECT_EQ(plan->Removes, (std::vector<std::size_t>{ 8, 7, 6, 5, 4, 3, 16, 15, 14, 13, 12, 11 }));
}

TEST(BotRosterTests, SevenRedEightBlueToEightRedSevenBlueMovesOne)
{
    const auto plan{ PlanRoster(MakeBots({ 7, 8 }), MakeRoster({ 8, 7 }), 0, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->Moves, (std::vector<RosterMove>{ RosterMove{ .Number = 15, .Team = 0 } }));
    EXPECT_TRUE(plan->Removes.empty());
    EXPECT_TRUE(plan->Adds.empty());
}

TEST(BotRosterTests, TwoVersusTwoToSixVersusSixAddsEight)
{
    const auto plan{ PlanRoster(MakeBots({ 2, 2 }), MakeRoster({ 6, 6 }), 0, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_TRUE(plan->Moves.empty());
    EXPECT_TRUE(plan->Removes.empty());
    EXPECT_EQ(plan->Adds, (std::vector<std::int8_t>{ 0, 0, 0, 0, 1, 1, 1, 1 }));
}

TEST(BotRosterTests, OneRedBotToThreeBlueMovesOneAndAddsTwo)
{
    const auto plan{ PlanRoster(MakeBots({ 1 }), MakeRoster({ 0, 3 }), 0, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->Moves, (std::vector<RosterMove>{ RosterMove{ .Number = 1, .Team = 1 } }));
    EXPECT_TRUE(plan->Removes.empty());
    EXPECT_EQ(plan->Adds, (std::vector<std::int8_t>{ 1, 1 }));
}

TEST(BotRosterTests, ExtraBotsMoveBeforeTheyAreRemoved)
{
    // Red has 4 and wants 1, blue has 0 and wants 2, green has 0 and wants 0: two bots move to blue, one is removed.
    const auto plan{ PlanRoster(MakeBots({ 4 }), MakeRoster({ 1, 2 }), 0, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->Moves, (std::vector<RosterMove>{ RosterMove{ .Number = 4, .Team = 1 }, RosterMove{ .Number = 3, .Team = 1 } }));
    EXPECT_EQ(plan->Removes, (std::vector<std::size_t>{ 2 }));
    EXPECT_TRUE(plan->Adds.empty());
}

TEST(BotRosterTests, AutoGoesToTheTeamOfTheLocalPlayer)
{
    const auto plan{ PlanRoster({}, MakeRoster({ 2 }, 3), 1, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->Adds, (std::vector<std::int8_t>{ 0, 0, 1, 1, 1 }));

    const auto other{ PlanRoster({}, MakeRoster({}, 2), 0, 1) };

    ASSERT_TRUE(other.has_value());
    EXPECT_EQ(other->Adds, (std::vector<std::int8_t>{ 0, 0 }));
}

TEST(BotRosterTests, AutoAddsToTheCountOfTheTeamOfTheLocalPlayer)
{
    // The local player is on team 1 with 2 bots already: a roster of 1 on team 1 plus 2 auto wants 3 there.
    const auto plan{ PlanRoster(MakeBots({ 0, 2 }), MakeRoster({ 0, 1 }, 2), 1, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->Adds, (std::vector<std::int8_t>{ 1 }));
}

TEST(BotRosterTests, AutoWithoutATeamOfTheLocalPlayerIsRejected)
{
    EXPECT_FALSE(PlanRoster({}, MakeRoster({}, 1), -1, 1).has_value());
    EXPECT_FALSE(PlanRoster({}, MakeRoster({}, 1), static_cast<std::int8_t>(k_MaxTeams), 1).has_value());

    // Without auto bots the team of the local player does not matter.
    EXPECT_TRUE(PlanRoster({}, MakeRoster({ 1 }), -1, 1).has_value());
}

TEST(BotRosterTests, RosterThatDoesNotFitIsRejectedWhole)
{
    // 8 + 8 bots with the local player: 17 players.
    const auto tooMany{ PlanRoster(MakeBots({ 2 }), MakeRoster({ 8, 8 }), 0, 1) };
    EXPECT_FALSE(tooMany.has_value());

    // 8 + 7 bots with the local player: 16 players, it fits.
    EXPECT_TRUE(PlanRoster(MakeBots({ 2 }), MakeRoster({ 8, 7 }), 0, 1).has_value());

    // More bots than machine slots, whatever the table.
    Roster huge{};
    huge.Teams.fill(k_MaxBots / k_MaxTeams + 1);
    EXPECT_FALSE(PlanRoster({}, huge, 0, 0).has_value());
}

TEST(BotRosterTests, PlanDoesNotDependOnTheOrderOfTheBots)
{
    std::vector<RosterBot> bots{ MakeBots({ 5, 5 }) };
    const auto first{ PlanRoster(bots, MakeRoster({ 3, 6 }), 0, 1) };

    std::ranges::reverse(bots);
    const auto second{ PlanRoster(bots, MakeRoster({ 3, 6 }), 0, 1) };

    ASSERT_TRUE(first.has_value());
    EXPECT_EQ(first, second);
    EXPECT_EQ(first->Moves, (std::vector<RosterMove>{ RosterMove{ .Number = 5, .Team = 1 } }));
    EXPECT_EQ(first->Removes, (std::vector<std::size_t>{ 4 }));
}

TEST(BotRosterTests, FreeBotNumberIsTheLowestOneNotInUse)
{
    EXPECT_EQ(FindFreeBotNumber({}), 1u);

    const std::vector<std::size_t> all{ 1, 2, 3, 4 };
    EXPECT_EQ(FindFreeBotNumber(all), 5u);

    // A bot removed from the middle leaves its number for the next one, whatever the order of the list.
    const std::vector<std::size_t> hole{ 4, 1, 3, 5 };
    EXPECT_EQ(FindFreeBotNumber(hole), 2u);
}

TEST(BotRosterTests, FreeBotNumberStopsAtTheLimit)
{
    std::vector<std::size_t> used{};
    for (std::size_t number{ 1 }; number <= k_MaxBots; ++number) used.push_back(number);

    EXPECT_FALSE(FindFreeBotNumber(used).has_value());

    used.erase(used.begin() + 40);
    EXPECT_EQ(FindFreeBotNumber(used), 41u);
}

TEST(BotRosterTests, PlayerUpdateHasTheIndexTheIdentifierAndTheTypeAtTheirOffsets)
{
    constexpr std::uint64_t identifier{ 0x4152544500000007 };

    for (const std::int32_t type : { k_PlayerUpdateLeft, k_PlayerUpdateRemove, k_PlayerUpdateAdded })
    {
        const auto update{ MakePlayerUpdate(13, identifier, type) };

        ASSERT_EQ(update.size(), k_PlayerUpdateSize);
        EXPECT_EQ(k_PlayerUpdateSize, 0xB8u);

        std::int32_t index{};
        std::uint64_t id{};
        std::int32_t kind{};
        std::memcpy(&index, update.data() + k_UpdatePlayerIndex, sizeof(index));
        std::memcpy(&id, update.data() + k_UpdateIdentifier, sizeof(id));
        std::memcpy(&kind, update.data() + k_UpdateType, sizeof(kind));

        EXPECT_EQ(index, 13);
        EXPECT_EQ(id, identifier);
        EXPECT_EQ(kind, type);
    }

    EXPECT_EQ(k_UpdatePlayerIndex, 0x0u);
    EXPECT_EQ(k_UpdateIdentifier, 0x4u);
    EXPECT_EQ(k_UpdateType, 0xCu);
    EXPECT_EQ(k_PlayerUpdateLeft, 0);
    EXPECT_EQ(k_PlayerUpdateRemove, 2);
}

TEST(BotRosterTests, PlayerUpdateLeavesEveryOtherByteZero)
{
    const auto update{ MakePlayerUpdate(15, 0x4152544500000001, k_PlayerUpdateRemove) };

    // The index, the identifier and the type take bytes 0x0 to 0xF.
    for (std::size_t offset{ 0x10 }; offset < update.size(); ++offset)
    {
        EXPECT_EQ(update[offset], 0) << "offset 0x" << std::hex << offset;
    }
}

TEST(BotRosterTests, StoreRemovesABotByNumberAndKeepsTheOthers)
{
    BotStore store{};
    store.AddBot(Bot{ .Number = 1, .PlayerIndex = 15 });
    store.AddBot(Bot{ .Number = 2, .PlayerIndex = 14 });
    store.AddBot(Bot{ .Number = 3, .PlayerIndex = 13 });

    store.RemoveBot(2);
    store.RemoveBot(9);

    std::vector<Bot> bots{};
    store.CopyBots(bots);

    ASSERT_EQ(bots.size(), 2u);
    EXPECT_EQ(bots[0].Number, 1u);
    EXPECT_EQ(bots[1].Number, 3u);
}

TEST(BotRosterTests, StoreTakesRemoveRequestsOnceAndCleanupDropsThem)
{
    BotStore store{};
    store.RequestRemove(4);
    store.RequestRemove(2);

    EXPECT_EQ(store.TakeRemoveRequests(), (std::vector<std::size_t>{ 4, 2 }));
    EXPECT_TRUE(store.TakeRemoveRequests().empty());

    store.RequestRemove(7);
    store.Cleanup();

    EXPECT_TRUE(store.TakeRemoveRequests().empty());
}

TEST(BotRosterTests, FreeNumbersFromTheStoreReuseTheMachineOfARemovedBot)
{
    BotStore store{};
    for (std::size_t number{ 1 }; number <= 6; ++number) store.AddBot(Bot{ .Number = number, .PlayerIndex = 16 - number });

    store.RemoveBot(5);

    std::vector<Bot> bots{};
    store.CopyBots(bots);

    std::vector<std::size_t> used{};
    for (const Bot& bot : bots) used.push_back(bot.Number);

    EXPECT_EQ(FindFreeBotNumber(used), 5u);
}

TEST(BotRosterTests, OneRedAndOneBlueToThreeBlueMovesOneAndAddsOne)
{
    const auto plan{ PlanRoster(MakeBots({ 1, 1 }), MakeRoster({ 0, 3 }), 0, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->Moves, (std::vector<RosterMove>{ RosterMove{ .Number = 1, .Team = 1 } }));
    EXPECT_TRUE(plan->Removes.empty());
    EXPECT_EQ(plan->Adds, (std::vector<std::int8_t>{ 1 }));
}

TEST(BotRosterTests, RosterTextListsTheTeamsWithBots)
{
    EXPECT_EQ(ToText(MakeRoster({ 7, 8 })), "0:7 1:8");
    EXPECT_EQ(ToText(MakeRoster({ 0, 0, 2 }, 3)), "2:2 auto:3");
    EXPECT_EQ(ToText(Roster{}), "empty");
}

TEST(BotRosterTests, FirstTickOfTheMatchLoadsWhateverTheRound)
{
    EXPECT_EQ(GetRosterStep(false, false, std::nullopt), RosterStep::Load);
    EXPECT_EQ(GetRosterStep(false, true, 1), RosterStep::Load);
    EXPECT_EQ(GetRosterStep(false, false, k_RoundStateEnded), RosterStep::Load);
}

TEST(BotRosterTests, RequestOfTheMatchWaitsForTheEndOfTheRound)
{
    EXPECT_EQ(GetRosterStep(true, true, 1), RosterStep::Nothing);
    EXPECT_EQ(GetRosterStep(true, true, 0), RosterStep::Nothing);
    EXPECT_EQ(GetRosterStep(true, true, std::nullopt), RosterStep::Nothing);
    EXPECT_EQ(GetRosterStep(true, true, k_RoundStateEnded), RosterStep::Break);
    EXPECT_EQ(GetRosterStep(true, true, k_RoundStateBetween), RosterStep::Break);
}

TEST(BotRosterTests, WithoutARequestThereIsNothingToDoDuringTheBreak)
{
    EXPECT_EQ(GetRosterStep(true, false, k_RoundStateEnded), RosterStep::Nothing);
    EXPECT_EQ(GetRosterStep(true, false, k_RoundStateBetween), RosterStep::Nothing);
}

TEST(BotRosterTests, BotGoneFromTheEngineIsRemovedNeverMovedAndTheTeamThatMissesGetsANewBot)
{
    std::vector<RosterBot> bots{ MakeBots({ 1 }) };
    bots.push_back(RosterBot{ .Number = 2, .PlayerIndex = 5, .Team = 12 });
    bots.push_back(RosterBot{ .Number = 3, .PlayerIndex = 4, .Team = -1 });

    const auto plan{ PlanRoster(bots, MakeRoster({ 1, 1 }), 0, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_TRUE(plan->Moves.empty());
    EXPECT_EQ(plan->Removes, (std::vector<std::size_t>{ 3, 2 }));
    EXPECT_EQ(plan->Adds, (std::vector<std::int8_t>{ 1 }));
}

TEST(BotRosterTests, GoneBotsAreRemovedAfterTheExtraBotsOfTheTeams)
{
    std::vector<RosterBot> bots{ MakeBots({ 3 }) };
    bots.push_back(RosterBot{ .Number = 4, .PlayerIndex = 5, .Team = -1 });

    const auto plan{ PlanRoster(bots, MakeRoster({ 1 }), 0, 1) };

    ASSERT_TRUE(plan.has_value());
    EXPECT_TRUE(plan->Moves.empty());
    EXPECT_EQ(plan->Removes, (std::vector<std::size_t>{ 3, 2, 4 }));
    EXPECT_TRUE(plan->Adds.empty());
}

TEST(BotRosterTests, RequestIsTakenOnceAndTheResultStaysPendingUntilItIsFinished)
{
    BotStore store{};
    EXPECT_FALSE(store.HasPendingRoster());
    EXPECT_EQ(store.GetRosterResult().Outcome, RosterOutcome::None);

    store.RequestRoster(MakeRoster({ 1, 2 }));

    EXPECT_TRUE(store.HasPendingRoster());
    EXPECT_EQ(store.GetRosterResult().Outcome, RosterOutcome::Pending);

    const auto request{ store.TakePendingRoster() };
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->Value, MakeRoster({ 1, 2 }));
    EXPECT_FALSE(store.HasPendingRoster());
    EXPECT_FALSE(store.TakePendingRoster().has_value());
    EXPECT_EQ(store.GetRosterResult().Outcome, RosterOutcome::Pending);
    EXPECT_FALSE(store.GetActiveRoster().has_value());

    store.FinishRoster(*request, RosterResult{ .Outcome = RosterOutcome::Applied, .Round = 2 }, true);

    EXPECT_EQ(store.GetRosterResult(), (RosterResult{ .Outcome = RosterOutcome::Applied, .Round = 2 }));
    EXPECT_EQ(store.GetActiveRoster(), MakeRoster({ 1, 2 }));
}

TEST(BotRosterTests, RejectedRequestLeavesTheActiveRosterAsItWas)
{
    BotStore store{};
    store.RequestRoster(MakeRoster({ 7, 8 }));
    store.FinishRoster(*store.TakeLoadRoster(), RosterResult{ .Outcome = RosterOutcome::AtLoad, .Round = 0 }, true);

    store.RequestRoster(MakeRoster({ 9, 9 }));
    const auto request{ store.TakePendingRoster() };
    store.FinishRoster(*request, RosterResult{ .Outcome = RosterOutcome::Rejected, .Reason = "does not fit" }, false);

    EXPECT_EQ(store.GetActiveRoster(), MakeRoster({ 7, 8 }));
    EXPECT_EQ(store.GetRosterResult().Outcome, RosterOutcome::Rejected);
    EXPECT_EQ(store.GetRosterResult().Reason, "does not fit");
}

TEST(BotRosterTests, LoadTakesThePendingRequestWithoutMakingItActiveBeforeItIsApplied)
{
    BotStore store{};
    store.RequestRoster(MakeRoster({ 7, 8 }));

    const auto request{ store.TakeLoadRoster() };

    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->Value, MakeRoster({ 7, 8 }));
    EXPECT_FALSE(store.HasPendingRoster());
    EXPECT_FALSE(store.GetActiveRoster().has_value());
    EXPECT_FALSE(store.TakeLoadRoster().has_value());
}

TEST(BotRosterTests, LoadTakesTheActiveRosterWhenNothingIsPending)
{
    BotStore store{};
    store.RequestRoster(MakeRoster({ 7, 8 }));
    store.FinishRoster(*store.TakeLoadRoster(), RosterResult{ .Outcome = RosterOutcome::AtLoad }, true);

    const auto request{ store.TakeLoadRoster() };

    ASSERT_TRUE(request.has_value());
    EXPECT_EQ(request->Value, MakeRoster({ 7, 8 }));
    EXPECT_EQ(store.GetActiveRoster(), MakeRoster({ 7, 8 }));
}

TEST(BotRosterTests, ResultOfAnOlderRequestNeverReplacesTheOneOfANewerRequest)
{
    BotStore store{};
    store.RequestRoster(MakeRoster({ 1 }));
    const auto older{ store.TakePendingRoster() };

    store.RequestRoster(MakeRoster({ 2 }));
    store.FinishRoster(*older, RosterResult{ .Outcome = RosterOutcome::Applied, .Round = 1 }, true);

    // The engine holds the older roster now, but the newer request is still pending and shows as such.
    EXPECT_EQ(store.GetRosterResult().Outcome, RosterOutcome::Pending);
    EXPECT_EQ(store.GetActiveRoster(), MakeRoster({ 1 }));
    EXPECT_TRUE(store.HasPendingRoster());

    const auto newer{ store.TakePendingRoster() };
    ASSERT_TRUE(newer.has_value());
    EXPECT_GT(newer->Sequence, older->Sequence);

    store.FinishRoster(*newer, RosterResult{ .Outcome = RosterOutcome::Applied, .Round = 2 }, true);
    EXPECT_EQ(store.GetRosterResult().Round, 2);
    EXPECT_EQ(store.GetActiveRoster(), MakeRoster({ 2 }));
}

TEST(BotRosterTests, NewerRequestReplacesAnOlderOneNotTakenYet)
{
    BotStore store{};
    store.RequestRoster(MakeRoster({ 1 }));
    store.RequestRoster(MakeRoster({ 0, 4 }));

    EXPECT_EQ(store.TakePendingRoster()->Value, MakeRoster({ 0, 4 }));
}

TEST(BotRosterTests, CleanupKeepsTheActiveRosterAndAPendingOneButNotTheBotsNorTheOldResult)
{
    BotStore store{};
    store.RequestRoster(MakeRoster({ 7, 8 }));
    store.FinishRoster(*store.TakePendingRoster(), RosterResult{ .Outcome = RosterOutcome::Applied, .Round = 3 }, true);
    store.RequestRoster(MakeRoster({ 1, 2 }));
    store.AddBot(Bot{ .Number = 1, .PlayerIndex = 15 });
    store.RequestRemove(1);
    store.SetMatchStarted();

    store.Cleanup();

    EXPECT_EQ(store.GetActiveRoster(), MakeRoster({ 7, 8 }));
    EXPECT_TRUE(store.HasPendingRoster());
    EXPECT_EQ(store.GetRosterResult(), RosterResult{ .Outcome = RosterOutcome::Pending });
    EXPECT_FALSE(store.IsMatchStarted());
    EXPECT_TRUE(store.TakeRemoveRequests().empty());

    std::vector<Bot> bots{};
    store.CopyBots(bots);
    EXPECT_TRUE(bots.empty());
}

TEST(BotRosterTests, CleanupWithoutAPendingRequestResetsTheResultButKeepsTheActiveRoster)
{
    BotStore store{};
    store.RequestRoster(MakeRoster({ 7, 8 }));
    store.FinishRoster(*store.TakePendingRoster(), RosterResult{ .Outcome = RosterOutcome::Applied, .Round = 3 }, true);

    store.Cleanup();

    EXPECT_EQ(store.GetActiveRoster(), MakeRoster({ 7, 8 }));
    EXPECT_EQ(store.GetRosterResult(), RosterResult{});
}

TEST(BotRosterTests, ClearRosterForgetsTheActiveThePendingAndTheResult)
{
    BotStore store{};
    store.RequestRoster(MakeRoster({ 7, 8 }));
    store.FinishRoster(*store.TakePendingRoster(), RosterResult{ .Outcome = RosterOutcome::AtLoad }, true);
    store.RequestRoster(MakeRoster({ 1, 2 }));

    store.ClearRoster();

    EXPECT_FALSE(store.GetActiveRoster().has_value());
    EXPECT_FALSE(store.HasPendingRoster());
    EXPECT_EQ(store.GetRosterResult(), RosterResult{});
}
