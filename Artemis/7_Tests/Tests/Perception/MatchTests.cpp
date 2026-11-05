#include <gtest/gtest.h>

import Common.Semantic.Type;
import Blam.Runtime.Game.Type;
import Perception.Match.Type;
import Tests.Support.Pipeline;
import std;

// The phase of the match (D103) is derived from the GameOver event of the engine ring, not from the raw phase of the game engine globals, which also
// reads Ended between the end of a round and the next one and for one tick at the death of the local player (ctf_c 4281).

namespace
{
    using Common::Semantic::Type::MatchPhase;
    using Common::Semantic::Type::RoundState;
    using Blam::Runtime::Game::Type::Alive::AliveEvent;
    using Blam::Runtime::Game::Type::Alive::AliveGame;
    using Perception::Match::Type::MatchStatus;
    using Tests::Support::Pipeline::Chain;

    constexpr std::uint32_t k_GameOverKey{ 0x780022 };
    constexpr std::uint32_t k_KillKey{ 0x780000 };

    class MatchTests : public ::testing::Test
    {
    protected:
        // Runs the Events and the Match services for one tick of the engine, with the entries given in the ring in that tick.
        auto Tick(std::uint32_t gameTick, std::vector<AliveEvent> events = {}, RoundState round = RoundState::InPlay, bool hasEngineGlobals = true) -> std::shared_ptr<const MatchStatus>
        {
            AliveGame game{};
            game.EngineTick = gameTick;
            game.HasEngineGlobals = hasEngineGlobals;
            game.RoundState = static_cast<std::uint8_t>(round);
            for (AliveEvent& event : events) game.Events.push_back(event);
            m_Chain.m_Tables.m_GameStore.Publish(std::move(game));

            m_Chain.m_Environment.m_EventsService.Update();
            m_Chain.m_Environment.m_MatchService.Update();
            return m_Chain.m_Environment.m_MatchStore.Acquire();
        }

        static auto Event(std::uint32_t sequence, std::uint32_t key, std::uint32_t gameTick) -> AliveEvent
        {
            return AliveEvent{ .Sequence = sequence, .GameTick = gameTick, .Key = key };
        }

        Chain m_Chain{};
    };
}

TEST_F(MatchTests, ThePhaseIsInGameFromTheStartOfTheMatch)
{
    const auto match{ this->Tick(100) };
    ASSERT_NE(match, nullptr);
    EXPECT_EQ(match->Phase, MatchPhase::InGame);
    EXPECT_EQ(this->Tick(101, { Event(1, k_KillKey, 101) })->Phase, MatchPhase::InGame);
}

TEST_F(MatchTests, TheEndOfARoundAndTheDeathOfThePlayerDoNotEndTheMatch)
{
    EXPECT_EQ(this->Tick(100)->Phase, MatchPhase::InGame);
    EXPECT_EQ(this->Tick(101, {}, RoundState::Ended)->Phase, MatchPhase::InGame);
    EXPECT_EQ(this->Tick(102, {}, RoundState::Between)->Phase, MatchPhase::InGame);
    EXPECT_EQ(this->Tick(103, { Event(1, k_KillKey, 103) })->Phase, MatchPhase::InGame);
    EXPECT_EQ(this->Tick(104)->Phase, MatchPhase::InGame);
}

TEST_F(MatchTests, GameOverInTheRingIsEndedThatTickAndAfter)
{
    EXPECT_EQ(this->Tick(100, { Event(1, k_KillKey, 100) })->Phase, MatchPhase::InGame);
    EXPECT_EQ(this->Tick(101)->Phase, MatchPhase::InGame);
    EXPECT_EQ(this->Tick(102, { Event(2, k_GameOverKey, 102) }, RoundState::Ended)->Phase, MatchPhase::Ended);
    EXPECT_EQ(this->Tick(103, {}, RoundState::Ended)->Phase, MatchPhase::Ended);
    EXPECT_EQ(this->Tick(104, { Event(3, k_KillKey, 104) })->Phase, MatchPhase::Ended);
}

TEST_F(MatchTests, ANewMatchStartsInGameAgain)
{
    EXPECT_EQ(this->Tick(500, { Event(5, k_GameOverKey, 500) })->Phase, MatchPhase::Ended);
    EXPECT_EQ(this->Tick(501)->Phase, MatchPhase::Ended);

    // The engine starts again: the clock goes back and the ring starts at 1.
    EXPECT_EQ(this->Tick(10, { Event(1, k_KillKey, 10) })->Phase, MatchPhase::InGame);
    EXPECT_EQ(this->Tick(600)->Phase, MatchPhase::InGame);
}

TEST_F(MatchTests, WithoutTheGameEngineGlobalsThePhaseIsUnknown)
{
    EXPECT_EQ(this->Tick(100, {}, RoundState::InPlay, false)->Phase, MatchPhase::Unknown);
    EXPECT_EQ(this->Tick(101)->Phase, MatchPhase::InGame);
}