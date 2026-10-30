#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.Runtime.Game.Type;
import Perception.Details.Type;
import Perception.Events.Type;
import Tests.Support.Pipeline;
import std;

// B4h H.3: the flag events Artemis generates from the transitions of the ObjectiveDetails, keyed by base team (D251), with ObjectiveDetails
// published by hand. The real sequence of events is checked by ReplayTests.CtfFlagEventsMatchTheExpectedList.

namespace
{
    using Common::Semantic::Type::Event;
    using Common::Semantic::Type::FlagState;
    using Common::Team::Type::Team;
    using Blam::Runtime::Game::Type::Alive::AliveEvent;
    using Blam::Runtime::Game::Type::Alive::AliveGame;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
    using Tests::Support::Pipeline::Chain;

    using Common::Engine::Type::k_NoHandle;

    constexpr std::uint32_t k_FlagA{ 0x1001 };
    constexpr std::uint32_t k_FlagB{ 0x1002 };
    constexpr std::uint32_t k_Player{ 0x2001 };
    constexpr std::uint32_t k_Biped{ 0x3001 };
    constexpr std::uint32_t k_FlagTakenKey{ 0x79E0 };
    constexpr std::uint32_t k_KillKey{ 0x780000 };

    class FlagEventsTests : public ::testing::Test
    {
    protected:
        // Runs one tick with the flag of the blue base in the given state.
        auto Tick(std::uint32_t handle, FlagState state, std::uint8_t allies = 0, std::int16_t round = 0, bool hasFlag = true) -> void
        {
            ++m_Tick;

            ObjectiveDetails details{};
            if (hasFlag)
            {
                ObjectiveDetail flag{ .Handle = handle, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red, .State = state, .AlliesInZone = allies };
                if (state == FlagState::Carried)
                {
                    flag.CarrierPlayer = k_Player;
                    flag.CarrierBiped = k_Biped;
                }
                details.Flags.emplace(handle, flag);
            }
            m_Chain.m_Environment.m_ObjectiveDetailsStore.Publish(std::move(details));

            AliveGame game{ m_Game };
            game.EngineTick = m_Tick;
            game.RoundIndex = round;
            m_Chain.m_Tables.m_GameStore.Publish(std::move(game));
            m_Game.Events.clear();

            m_Chain.m_Environment.m_EventsService.Update();
        }

        auto Log() const -> std::vector<Perception::Events::Type::GameEvent>
        {
            const auto log{ m_Chain.m_Environment.m_EventsStore.Acquire() };
            return log ? log->Rows : std::vector<Perception::Events::Type::GameEvent>{};
        }

        Chain m_Chain{};
        AliveGame m_Game{};
        std::uint32_t m_Tick{ 99 };
    };
}

TEST_F(FlagEventsTests, FirstTickOfALoadGeneratesNothing)
{
    this->Tick(k_FlagA, FlagState::Carried);

    EXPECT_TRUE(this->Log().empty());
}

TEST_F(FlagEventsTests, TakenAndDroppedCarryTheCarrierAndTheFlag)
{
    this->Tick(k_FlagA, FlagState::Home);
    this->Tick(k_FlagA, FlagState::Carried);
    this->Tick(k_FlagA, FlagState::Carried);
    this->Tick(k_FlagA, FlagState::Dropped);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[0].GameTick, 101u);
    EXPECT_EQ(rows[0].Sequence, 1u);
    EXPECT_EQ(rows[0].EngineSequence, 0u);
    EXPECT_EQ(rows[0].Key, 0u);
    EXPECT_EQ(rows[0].CausePlayer, k_Player);
    EXPECT_EQ(rows[0].CauseObject, k_Biped);
    EXPECT_EQ(rows[0].EffectObject, k_FlagA);
    EXPECT_EQ(rows[0].Modifier, static_cast<std::uint8_t>(Team::Red));
    EXPECT_EQ(rows[1].Kind, Event::FlagDropped);
    EXPECT_EQ(rows[1].GameTick, 103u);
    EXPECT_EQ(rows[1].Sequence, 2u);
    EXPECT_EQ(rows[1].CausePlayer, k_Player);
}

TEST_F(FlagEventsTests, DroppedFlagRecreatedAtHomeIsReturnedWithAlliesAndResetWithout)
{
    this->Tick(k_FlagA, FlagState::Dropped, 1);
    this->Tick(k_FlagB, FlagState::Home);
    this->Tick(k_FlagB, FlagState::Dropped, 0);
    this->Tick(k_FlagA, FlagState::Home);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].Kind, Event::FlagReturned);
    EXPECT_EQ(rows[0].EffectObject, k_FlagA);
    EXPECT_EQ(rows[0].CausePlayer, k_NoHandle);
    EXPECT_EQ(rows[1].Kind, Event::FlagReset);
    EXPECT_EQ(rows[1].EffectObject, k_FlagB);
}

TEST_F(FlagEventsTests, CarriedFlagRecreatedOrGoneIsCapturedOnce)
{
    this->Tick(k_FlagA, FlagState::Carried);
    m_Game.TeamScores[0].Total = 1;
    this->Tick(k_FlagB, FlagState::Home);
    this->Tick(k_FlagB, FlagState::Home);

    auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].Kind, Event::FlagCaptured);
    EXPECT_EQ(rows[0].CausePlayer, k_Player);
    EXPECT_EQ(rows[0].EffectObject, k_FlagA);

    // Gone for a tick and back: one capture, not two.
    this->Tick(k_FlagB, FlagState::Carried);
    m_Game.TeamScores[0].Total = 2;
    this->Tick(k_FlagB, FlagState::Carried, 0, 0, false);
    this->Tick(k_FlagA, FlagState::Home);
    rows = this->Log();
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[1].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[2].Kind, Event::FlagCaptured);
}

TEST_F(FlagEventsTests, CarriedFlagThatGoesHomeWithoutAScoreIsAResetNotACapture)
{
    this->Tick(k_FlagA, FlagState::Carried);
    this->Tick(k_FlagA, FlagState::Carried);
    this->Tick(k_FlagB, FlagState::Home);
    EXPECT_TRUE(this->Log().empty());

    // The next tick brings no score: the pending capture becomes a reset with the tick of the transition.
    this->Tick(k_FlagB, FlagState::Home);
    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].Kind, Event::FlagReset);
    EXPECT_EQ(rows[0].GameTick, 102u);
    EXPECT_EQ(rows[0].EffectObject, k_FlagA);
    EXPECT_EQ(rows[0].CausePlayer, k_NoHandle);
}

TEST_F(FlagEventsTests, ScoreOneTickAfterTheFlagWentHomeStillMakesItACapture)
{
    this->Tick(k_FlagA, FlagState::Carried);
    this->Tick(k_FlagB, FlagState::Home);
    m_Game.TeamScores[1].Round = 1;
    this->Tick(k_FlagB, FlagState::Home);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].Kind, Event::FlagCaptured);
    EXPECT_EQ(rows[0].GameTick, 101u);
    EXPECT_EQ(rows[0].CausePlayer, k_Player);
}

TEST_F(FlagEventsTests, ChangeOfRoundGeneratesNoFlagEvents)
{
    this->Tick(k_FlagA, FlagState::Carried, 0, 0);
    this->Tick(k_FlagB, FlagState::Home, 0, 1);
    this->Tick(k_FlagB, FlagState::Carried, 0, 1);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[0].GameTick, 102u);
}

TEST_F(FlagEventsTests, EngineFlagEventsAreLeftOutAndTheSequencesAreSeparate)
{
    this->Tick(k_FlagA, FlagState::Home);

    m_Game.Events.push_back(AliveEvent{ .Sequence = 7, .GameTick = 100, .Key = k_KillKey });
    m_Game.Events.push_back(AliveEvent{ .Sequence = 8, .GameTick = 100, .Key = k_FlagTakenKey });
    this->Tick(k_FlagA, FlagState::Carried);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 2u);
    // The engine event of the tick comes first, then the flag event; both numbered by Artemis from 1.
    EXPECT_EQ(rows[0].Kind, Event::Kill);
    EXPECT_EQ(rows[0].Sequence, 1u);
    EXPECT_EQ(rows[0].EngineSequence, 7u);
    EXPECT_EQ(rows[1].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[1].Sequence, 2u);
    EXPECT_EQ(rows[1].EngineSequence, 0u);
}