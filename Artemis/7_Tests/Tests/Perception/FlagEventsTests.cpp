#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Object.Type;
import Perception.Classifier.Type;
import Perception.Details.Type;
import Perception.Events.Type;
import Perception.Players.Type;
import Tests.Support.Pipeline;
import std;

// B4h H.3: the flag events Artemis generates from the transitions of the ObjectiveDetails, keyed by base team (D251), with ObjectiveDetails
// published by hand. The real sequence of events is checked by ReplayTests.CtfFlagEventsMatchTheExpectedList.

namespace
{
    namespace Subtype = Common::Semantic::Type::Subtype;

    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Event;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::FlagState;
    using Common::Team::Type::Team;
    using Blam::Runtime::Game::Type::Alive::AliveEvent;
    using Blam::Runtime::Game::Type::Alive::AliveGame;
    using Blam::Runtime::Game::Type::Alive::AliveMegaloObject;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Multiplayer::Multiplayer;
    using Blam::Runtime::Object::Type::Weapon::Weapon;
    using Perception::Classifier::Type::TagClassification;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::StandDetail;
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

        // Generates the flag events of a tick without pipeline (sparse mode) with the flag of the blue base in the given state.
        auto Light(std::uint32_t handle, FlagState state, std::uint8_t allies = 0) -> void
        {
            ++m_Tick;

            ObjectiveDetails details{};
            ObjectiveDetail flag{ .Handle = handle, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red, .State = state, .AlliesInZone = allies };
            if (state == FlagState::Carried)
            {
                flag.CarrierPlayer = k_Player;
                flag.CarrierBiped = k_Biped;
            }
            details.Flags.emplace(handle, flag);

            AliveGame game{ m_Game };
            game.EngineTick = m_Tick;
            m_Chain.m_Environment.m_EventsService.UpdateFlags(game, details);
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

TEST_F(FlagEventsTests, FlagRecreatedOnTheGroundIsComparedAsTheSameObject)
{
    constexpr std::uint32_t k_FlagC{ 0x1003 };
    constexpr std::uint32_t k_FlagD{ 0x1004 };

    // Taken, dropped (the engine recreates the flag with a new handle), lying on the ground (recreated again), taken again.
    this->Tick(k_FlagA, FlagState::Home);
    this->Tick(k_FlagA, FlagState::Carried);
    this->Tick(k_FlagB, FlagState::Dropped);
    this->Tick(k_FlagB, FlagState::Dropped);
    this->Tick(k_FlagC, FlagState::Dropped);
    this->Tick(k_FlagD, FlagState::Carried);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[0].EffectObject, k_FlagA);
    EXPECT_EQ(rows[1].Kind, Event::FlagDropped);
    EXPECT_EQ(rows[1].GameTick, 102u);
    EXPECT_EQ(rows[1].CausePlayer, k_Player);
    EXPECT_EQ(rows[1].EffectObject, k_FlagB);
    EXPECT_EQ(rows[2].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[2].GameTick, 105u);
    EXPECT_EQ(rows[2].EffectObject, k_FlagD);
}

TEST_F(FlagEventsTests, DroppedFlagRecreatedStillDroppedMakesNoReturnOrReset)
{
    this->Tick(k_FlagA, FlagState::Dropped, 2);
    this->Tick(k_FlagB, FlagState::Dropped, 2);

    EXPECT_TRUE(this->Log().empty());
}

// Q101.B: the flag events of the ticks without pipeline wait in the memory and join the log in the next full tick, ordered by tick.
TEST_F(FlagEventsTests, FlagEventsOfLightTicksJoinTheLogInTheNextFullTickOrderedByTickWithTheEngineEventsFirst)
{
    this->Tick(k_FlagA, FlagState::Home);
    this->Light(k_FlagA, FlagState::Carried);
    this->Light(k_FlagA, FlagState::Dropped);
    this->Light(k_FlagA, FlagState::Dropped);
    EXPECT_TRUE(this->Log().empty());

    // The engine event happened in the tick of the drop: it goes before the flag event of that tick.
    m_Game.Events.push_back(AliveEvent{ .Sequence = 7, .GameTick = 102, .Key = k_KillKey });
    this->Tick(k_FlagA, FlagState::Dropped);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 3u);
    EXPECT_EQ(rows[0].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[0].GameTick, 101u);
    EXPECT_EQ(rows[0].Sequence, 1u);
    EXPECT_EQ(rows[1].Kind, Event::Kill);
    EXPECT_EQ(rows[1].GameTick, 102u);
    EXPECT_EQ(rows[1].Sequence, 2u);
    EXPECT_EQ(rows[2].Kind, Event::FlagDropped);
    EXPECT_EQ(rows[2].GameTick, 102u);
    EXPECT_EQ(rows[2].Sequence, 3u);
    EXPECT_EQ(rows[2].CausePlayer, k_Player);
}

TEST_F(FlagEventsTests, ACaptureInALightTickKeepsItsTickAndTheFullTickAddsNothingTwice)
{
    this->Tick(k_FlagA, FlagState::Home);
    this->Light(k_FlagA, FlagState::Carried);
    m_Game.TeamScores[0].Total = 1;
    this->Light(k_FlagB, FlagState::Home);
    this->Tick(k_FlagB, FlagState::Home);
    this->Tick(k_FlagB, FlagState::Home);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[0].GameTick, 101u);
    EXPECT_EQ(rows[1].Kind, Event::FlagCaptured);
    EXPECT_EQ(rows[1].GameTick, 102u);
    EXPECT_EQ(rows[1].Sequence, 2u);
}

TEST_F(FlagEventsTests, LightEventsOfAnotherMatchAreForgotten)
{
    this->Tick(k_FlagA, FlagState::Home);
    m_Game.Events.push_back(AliveEvent{ .Sequence = 9, .GameTick = 100, .Key = k_KillKey });
    this->Tick(k_FlagA, FlagState::Home);
    this->Light(k_FlagA, FlagState::Carried);

    // The engine ring starts again at 1: a new match, so the light event of the old one is dropped.
    m_Game.Events.push_back(AliveEvent{ .Sequence = 1, .GameTick = 5, .Key = k_KillKey });
    this->Tick(k_FlagA, FlagState::Carried);

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].Kind, Event::Kill);
    EXPECT_EQ(rows[0].Sequence, 1u);
}

namespace
{
    constexpr std::uint32_t k_Stand{ 0x5001 };
    constexpr std::uint32_t k_FlagRecreated{ 0x1003 };

    // The flag pass over what a light snapshot decodes into, with what the last full tick published made by hand.
    class FlagPassTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            m_Chain.m_Environment.m_TagClassificationsStore.Add("weap", "test\\flag",
                TagClassification{ .Function = Function::ObjectiveItem, .Subtype = static_cast<std::uint8_t>(Subtype::ObjectiveItem::Flag) });
            m_Chain.m_Environment.m_TagClassificationsStore.Add("weap", "test\\rifle", TagClassification{ .Function = Function::Weapon });

            Players players{};
            players.Rows.push_back(Player{ .Handle = k_Player, .BipedHandle = k_Biped });
            players.Index.emplace(k_Player, 0);
            m_Chain.m_Environment.m_PlayersStore.Publish(std::move(players));
        }

        // Publishes what the last full tick left: the flag at home with the allies of its zone, and the stand.
        auto Decision(std::uint8_t allies) -> void
        {
            ObjectiveDetails details{};
            details.Flags.emplace(k_FlagA, ObjectiveDetail{ .Handle = k_FlagA, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red, .State = FlagState::Home,
                .HomeStand = k_Stand, .AlliesInZone = allies });
            details.Stands.emplace(k_Stand, StandDetail{ .Handle = k_Stand, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red, .WorldOrigin = Vec3{} });
            m_Chain.m_Environment.m_ObjectiveDetailsStore.Publish(std::move(details));

            // The events service starts from the flag at home, as after a full tick.
            AliveGame game{};
            game.EngineTick = m_Tick;
            m_Chain.m_Tables.m_GameStore.Publish(std::move(game));
            m_Chain.m_Environment.m_EventsService.Update();
        }

        // Decodes a light tick by hand: the weapons, and the megalo element of the first one when given. Then runs the flag pass.
        auto Light(std::initializer_list<std::pair<std::uint32_t, Weapon>> weapons, const Vec3& flagPosition,
            std::optional<AliveMegaloObject> element = std::nullopt) -> void
        {
            ++m_Tick;

            auto& objects{ m_Chain.m_Tables.m_LightObjectStore };
            for (const auto& [handle, weapon] : weapons)
            {
                objects.RemoveObject(handle);
                AliveObject object{};
                object.Handle = handle;
                object.FourCC = "weap";
                object.TagName = handle == k_Rifle ? "test\\rifle" : "test\\flag";
                object.WorldOrigin = flagPosition;
                object.Multiplayer = Multiplayer{ .Team = Team::Blue };
                object.Specific = weapon;
                objects.AddObject(handle, object);
            }
            objects.Publish();

            AliveGame game{};
            game.EngineTick = m_Tick;
            if (element) game.MegaloObjects.push_back(*element);
            m_Chain.m_Tables.m_LightGameStore.Publish(std::move(game));

            m_Chain.m_Environment.m_FlagPassService.Update();
        }

        auto Full() -> void
        {
            ++m_Tick;

            AliveGame game{};
            game.EngineTick = m_Tick;
            m_Chain.m_Tables.m_GameStore.Publish(std::move(game));
            m_Chain.m_Environment.m_EventsService.Update();
        }

        auto Log() const -> std::vector<Perception::Events::Type::GameEvent>
        {
            const auto log{ m_Chain.m_Environment.m_EventsStore.Acquire() };
            return log ? log->Rows : std::vector<Perception::Events::Type::GameEvent>{};
        }

        static constexpr std::uint32_t k_Rifle{ 0x1900 };

        Chain m_Chain{};
        std::uint32_t m_Tick{ 99 };
    };

    auto Carried() -> Weapon
    {
        Weapon weapon{};
        weapon.HolderUnit = k_Biped;
        return weapon;
    }
}

TEST_F(FlagPassTests, ATakeAndADropBetweenTwoDecisionsAreDatedInTheirOwnTicks)
{
    this->Decision(1);

    this->Light({ { k_FlagA, Carried() } }, Vec3{});
    this->Light({ { k_FlagA, Weapon{} } }, Vec3{ 30.0f, 0.0f, 0.0f });
    this->Light({ { k_FlagA, Weapon{} } }, Vec3{ 30.0f, 0.0f, 0.0f });
    EXPECT_TRUE(this->Log().empty());

    this->Full();

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 2u);
    EXPECT_EQ(rows[0].Kind, Event::FlagTaken);
    EXPECT_EQ(rows[0].GameTick, 100u);
    EXPECT_EQ(rows[0].CausePlayer, k_Player);
    EXPECT_EQ(rows[0].CauseObject, k_Biped);
    EXPECT_EQ(rows[1].Kind, Event::FlagDropped);
    EXPECT_EQ(rows[1].GameTick, 101u);
    EXPECT_EQ(rows[1].CausePlayer, k_Player);
}

TEST_F(FlagPassTests, ADroppedFlagRecreatedAtHomeIsReturnedWhenTheLastDecisionHadAlliesInTheZone)
{
    this->Decision(2);
    this->Light({ { k_FlagA, Weapon{} } }, Vec3{ 30.0f, 0.0f, 0.0f });

    // The engine recreates it at the stand with another handle and no megalo element.
    m_Chain.m_Tables.m_LightObjectStore.RemoveObject(k_FlagA);
    this->Light({ { k_FlagRecreated, Weapon{} } }, Vec3{});
    this->Full();

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].Kind, Event::FlagReturned);
    EXPECT_EQ(rows[0].GameTick, 101u);
    EXPECT_EQ(rows[0].EffectObject, k_FlagA);
}

TEST_F(FlagPassTests, ADroppedFlagRecreatedAtHomeIsResetWhenTheLastDecisionHadNoAllies)
{
    this->Decision(0);
    this->Light({ { k_FlagA, Weapon{} } }, Vec3{ 30.0f, 0.0f, 0.0f });

    m_Chain.m_Tables.m_LightObjectStore.RemoveObject(k_FlagA);
    this->Light({ { k_FlagRecreated, Weapon{} } }, Vec3{});
    this->Full();

    const auto rows{ this->Log() };
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0].Kind, Event::FlagReset);
    EXPECT_EQ(rows[0].GameTick, 101u);
}

TEST_F(FlagPassTests, WeaponsThatAreNotFlagsAndTagsNeverClassifiedGenerateNothing)
{
    this->Decision(0);

    Weapon rifle{};
    rifle.HolderUnit = k_Biped;
    this->Light({ { k_Rifle, rifle } }, Vec3{});
    this->Full();

    EXPECT_TRUE(this->Log().empty());
    EXPECT_TRUE(m_Chain.m_Environment.m_FlagPassService.BuildFlags().Flags.empty());
}

TEST_F(FlagPassTests, TheMegaloStateAndTheResetTimerOfALightFlagAreRead)
{
    this->Decision(1);

    this->Light({ { k_FlagA, Weapon{} } }, Vec3{ 30.0f, 0.0f, 0.0f }, AliveMegaloObject{ .ObjectHandle = k_FlagA, .State = 2 });
    const auto flags{ m_Chain.m_Environment.m_FlagPassService.BuildFlags() };

    ASSERT_TRUE(flags.Flags.contains(k_FlagA));
    EXPECT_EQ(flags.Flags.at(k_FlagA).State, FlagState::Dropped);
    EXPECT_EQ(flags.Flags.at(k_FlagA).HomeStand, k_Stand);
    EXPECT_EQ(flags.Flags.at(k_FlagA).AlliesInZone, 1u);
}

TEST_F(FlagPassTests, WithoutAFullTickBeforeThePassDoesNothing)
{
    this->Light({ { k_FlagA, Carried() } }, Vec3{});
    this->Full();

    EXPECT_TRUE(this->Log().empty());
}