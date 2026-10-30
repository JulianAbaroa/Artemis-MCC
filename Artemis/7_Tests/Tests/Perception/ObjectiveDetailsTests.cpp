#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Common.ZoneShape.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Object.Type;
import Perception.Classifier.Type;
import Perception.Details.Type;
import Perception.Entities.Type;
import Perception.Match.Type;
import Perception.Players.Type;
import Tests.Support.Pipeline;
import std;

// B4h H.2: the objective details (flags and stands) and the round of the match, from stores filled by hand. Where the real
// values come from is checked by ReplayTests.CtfObjectivesFollowTheRecordedMatch.

namespace
{
    namespace Subtype = Common::Semantic::Type::Subtype;

    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::FlagState;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::RoundState;
    using Common::Team::Type::Team;
    using Common::ZoneShape::Type::ZoneShape;
    using Common::ZoneShape::Type::ZoneShapeKind;
    using Blam::Runtime::Game::Type::Alive::AliveGame;
    using Blam::Runtime::Game::Type::Alive::AliveMegaloObject;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Weapon::Weapon;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Perception::Classifier::Type::Classified;
    using Perception::Classifier::Type::Classifieds;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ZoneDetail;
    using Perception::Details::Type::ZoneDetails;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Tests::Support::Pipeline::Chain;

    using Common::Engine::Type::k_NoHandle;

    constexpr std::uint32_t k_Flag{ 0x1001 };
    constexpr std::uint32_t k_Stand{ 0x1002 };
    constexpr std::uint32_t k_Zone{ 0x1003 };
    constexpr std::uint32_t k_Ally{ 0x2001 };
    constexpr std::uint32_t k_AllyBiped{ 0x3001 };
    constexpr std::uint32_t k_Enemy{ 0x2002 };
    constexpr std::uint32_t k_EnemyBiped{ 0x3002 };

    // The flag of the blue base (team 1). In this round the red side (team 0) plays it, so the owner is red.
    class ObjectiveDetailsTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            m_Game.TeamMapping = { 0xFF, 0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
            m_Game.TeamMapping[0] = 1;
            m_Game.TeamMapping[1] = 0;
            this->PublishWorld();
        }

        auto AddEntity(Entities& entities, std::uint32_t handle, Team team, const Vec3& origin) -> void
        {
            entities.Rows.push_back(Entity{ .Handle = handle, .Team = team, .WorldOrigin = origin, .Forward = Vec3{ 0.0f, 1.0f, 0.0f }, .Up = Vec3{ 0.0f, 0.0f, 1.0f } });
        }

        auto AddPlayer(Players& players, std::uint32_t handle, std::uint32_t biped, Team team) -> void
        {
            players.Rows.push_back(Player{ .Handle = handle, .Team = team, .BipedHandle = biped, .Body = Body{} });
        }

        // Publishes the tables from the members. The zone is a cylinder of radius 2 around (0, 0, 0).
        auto PublishWorld() -> void
        {
            AliveObject flag{};
            flag.Handle = k_Flag;
            Weapon weapon{};
            weapon.HolderUnit = m_Holder;
            flag.Specific = weapon;
            m_Chain.m_Tables.m_ObjectStore.RemoveObject(k_Flag);
            m_Chain.m_Tables.m_ObjectStore.AddObject(k_Flag, flag);
            m_Chain.m_Tables.m_ObjectStore.Publish();

            Classifieds classifieds{};
            classifieds.push_back(Classified{ .Handle = k_Flag, .Kind = ObjectKind::Weapon, .Function = Function::ObjectiveItem,
                .Subtype = static_cast<std::uint8_t>(Subtype::ObjectiveItem::Flag) });
            classifieds.push_back(Classified{ .Handle = k_Stand, .Kind = ObjectKind::Crate, .Function = Function::ObjectiveSpawn,
                .Subtype = static_cast<std::uint8_t>(Subtype::ObjectiveSpawn::FlagStand) });
            classifieds.push_back(Classified{ .Handle = k_Zone, .Kind = ObjectKind::Crate, .Function = Function::ObjectiveZone, .BoundTo = k_Flag });
            m_Chain.m_Environment.m_ClassifierStore.Publish(std::move(classifieds));

            Entities entities{};
            this->AddEntity(entities, k_Flag, Team::Blue, m_FlagPosition);
            this->AddEntity(entities, k_Stand, Team::Blue, Vec3{});
            this->AddEntity(entities, k_Zone, Team::Neutral, Vec3{});
            this->AddEntity(entities, k_AllyBiped, Team::Red, m_AllyPosition);
            this->AddEntity(entities, k_EnemyBiped, Team::Blue, Vec3{ 0.5f, 0.0f, 0.0f });
            std::ranges::sort(entities.Rows, {}, &Entity::Handle);
            for (std::size_t row{ 0 }; row < entities.Rows.size(); ++row) entities.Index.emplace(entities.Rows[row].Handle, row);
            m_Chain.m_Environment.m_EntitiesStore.Publish(std::move(entities));

            Players players{};
            this->AddPlayer(players, k_Ally, k_AllyBiped, Team::Red);
            this->AddPlayer(players, k_Enemy, k_EnemyBiped, Team::Blue);
            for (std::size_t row{ 0 }; row < players.Rows.size(); ++row) players.Index.emplace(players.Rows[row].Handle, row);
            m_Chain.m_Environment.m_PlayersStore.Publish(std::move(players));

            ZoneDetails zones{};
            zones.emplace(k_Zone, ZoneDetail{ .Handle = k_Zone, .Shape = ZoneShape{ .Radius = 2.0f, .Top = 2.0f, .Bottom = 0.0f, .Kind = ZoneShapeKind::Cylinder } });
            m_Chain.m_Environment.m_ZoneDetailsStore.Publish(std::move(zones));

            m_Chain.m_Tables.m_GameStore.Publish(AliveGame{ m_Game });
        }

        // Runs the service and returns the flag it published, or null.
        auto Update() -> const ObjectiveDetail*
        {
            this->PublishWorld();
            m_Chain.m_Environment.m_ObjectiveDetailsService.Update();
            m_Published = m_Chain.m_Environment.m_ObjectiveDetailsStore.Acquire();
            if (!m_Published) return nullptr;

            const auto it{ m_Published->Flags.find(k_Flag) };
            return it == m_Published->Flags.end() ? nullptr : &it->second;
        }

        Chain m_Chain{};
        AliveGame m_Game{};
        std::uint32_t m_Holder{ k_NoHandle };
        Vec3 m_AllyPosition{ 1.0f, 0.0f, 0.5f };
        // The stand is at the origin.
        Vec3 m_FlagPosition{};
        std::shared_ptr<const Perception::Details::Type::ObjectiveDetails> m_Published{};
    };
}

TEST_F(ObjectiveDetailsTests, FlagAtHomeKnowsItsBaseOwnerAndStand)
{
    m_Game.MegaloObjects.push_back(AliveMegaloObject{ .ObjectHandle = k_Flag, .State = 0 });

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Home);
    EXPECT_EQ(flag->BaseTeam, Team::Blue);
    EXPECT_EQ(flag->OwnerTeam, Team::Red);
    EXPECT_EQ(flag->HomeStand, k_Stand);
    EXPECT_EQ(flag->CarrierPlayer, k_NoHandle);
    EXPECT_FALSE(flag->IsResetRunning);

    const auto stand{ m_Published->Stands.find(k_Stand) };
    ASSERT_NE(stand, m_Published->Stands.end());
    EXPECT_EQ(stand->second.BaseTeam, Team::Blue);
    EXPECT_EQ(stand->second.OwnerTeam, Team::Red);
}

TEST_F(ObjectiveDetailsTests, SideNotInUseHasNoOwner)
{
    m_Game.TeamMapping[1] = 0xFF;

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->OwnerTeam, Team::Neutral);
    EXPECT_EQ(flag->AlliesInZone, 0);
}

TEST_F(ObjectiveDetailsTests, HolderUnitMakesItCarriedByThePlayerOfThatBiped)
{
    m_Holder = k_AllyBiped;
    m_Game.MegaloObjects.push_back(AliveMegaloObject{ .ObjectHandle = k_Flag, .State = 1 });

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Carried);
    EXPECT_EQ(flag->CarrierBiped, k_AllyBiped);
    EXPECT_EQ(flag->CarrierPlayer, k_Ally);
}

TEST_F(ObjectiveDetailsTests, CarrierThatIsNotAPlayerLeavesThePlayerEmpty)
{
    m_Holder = 0x3999;

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Carried);
    EXPECT_EQ(flag->CarrierBiped, 0x3999u);
    EXPECT_EQ(flag->CarrierPlayer, k_NoHandle);
}

TEST_F(ObjectiveDetailsTests, DroppedFlagReadsTheResetTimerOfItsOwnMegaloElement)
{
    // Another element copies the timer with another value: only the element of the flag counts.
    m_Game.MegaloObjects.push_back(AliveMegaloObject{ .ObjectHandle = 0x5555, .State = 2, .ResetTimer = 0x05000014 });
    m_Game.MegaloObjects.push_back(AliveMegaloObject{ .ObjectHandle = k_Flag, .State = 2, .ResetTimer = 0x07000FA1 });

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Dropped);
    EXPECT_TRUE(flag->IsResetRunning);
    // 0xFA1 = 4001 units of 1/1200 s at 20 per tick: rounded up to 201 ticks.
    EXPECT_EQ(flag->ResetTicks, 201u);
}

TEST_F(ObjectiveDetailsTests, PausedTimerIsNotRunning)
{
    m_Game.MegaloObjects.push_back(AliveMegaloObject{ .ObjectHandle = k_Flag, .State = 2, .ResetTimer = 72000 });

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_FALSE(flag->IsResetRunning);
    EXPECT_EQ(flag->ResetTicks, 3600u);
}

TEST_F(ObjectiveDetailsTests, WithoutMegaloTheFlagIsHomeOnlyWhenItIsAtItsStand)
{
    // On the stand (the same place, and within 1.0 both ways).
    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Home);

    m_FlagPosition = Vec3{ 0.7f, 0.0f, 0.9f };
    flag = this->Update();
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Home);

    m_Holder = k_AllyBiped;
    flag = this->Update();
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Carried);

    // Nobody holds it and it is off the stand: dropped, and it stays so (nothing is remembered from the tick before).
    m_Holder = k_NoHandle;
    m_FlagPosition = Vec3{ 3.0f, 0.0f, 0.0f };
    flag = this->Update();
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Dropped);
    EXPECT_EQ(flag->ResetTicks, 0u);

    flag = this->Update();
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Dropped);
}

TEST_F(ObjectiveDetailsTests, TheDistanceToTheStandIsCheckedHorizontallyAndVertically)
{
    m_FlagPosition = Vec3{ 0.0f, 0.0f, 1.5f };
    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Dropped);

    m_FlagPosition = Vec3{ 0.8f, 0.8f, 0.0f };
    flag = this->Update();
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Dropped);
}

TEST_F(ObjectiveDetailsTests, MegaloThatStillSaysCarriedWithoutAHolderFallsBackToThePosition)
{
    // A recreated flag lying on the ground: its element says carried for a few ticks while nobody holds it.
    m_Game.MegaloObjects.push_back(AliveMegaloObject{ .ObjectHandle = k_Flag, .State = 1 });
    m_FlagPosition = Vec3{ 5.0f, 0.0f, 0.0f };

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Dropped);
}

TEST_F(ObjectiveDetailsTests, ResetTimerIsOnlyReadFromAnElementThatSaysDropped)
{
    m_Game.MegaloObjects.push_back(AliveMegaloObject{ .ObjectHandle = k_Flag, .State = 1, .ResetTimer = 72000 });
    m_FlagPosition = Vec3{ 5.0f, 0.0f, 0.0f };

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Dropped);
    EXPECT_EQ(flag->ResetTicks, 0u);
    EXPECT_FALSE(flag->IsResetRunning);
}

TEST_F(ObjectiveDetailsTests, MegaloHomeWinsOverThePosition)
{
    m_Game.MegaloObjects.push_back(AliveMegaloObject{ .ObjectHandle = k_Flag, .State = 0 });
    m_FlagPosition = Vec3{ 5.0f, 0.0f, 0.0f };

    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->State, FlagState::Home);
}

TEST_F(ObjectiveDetailsTests, OnlyLiveBipedsOfTheOwnerTeamInsideTheZoneAreAllies)
{
    const ObjectiveDetail* flag{ this->Update() };
    ASSERT_NE(flag, nullptr);
    // The enemy is inside the zone but is not of the owner team.
    EXPECT_EQ(flag->AlliesInZone, 1);

    m_AllyPosition = Vec3{ 5.0f, 0.0f, 0.0f };
    flag = this->Update();
    ASSERT_NE(flag, nullptr);
    EXPECT_EQ(flag->AlliesInZone, 0);
}

TEST_F(ObjectiveDetailsTests, RoundAndTeamScoresComeFromTheGame)
{
    m_Game.RoundIndex = 2;
    m_Game.RoundState = 3;
    m_Game.TeamScores[1] = { .Round = 1, .Total = 4 };
    this->PublishWorld();
    m_Chain.m_Environment.m_MatchService.Update();

    const auto match{ m_Chain.m_Environment.m_MatchStore.Acquire() };
    ASSERT_NE(match, nullptr);
    EXPECT_EQ(match->RoundIndex, 2);
    EXPECT_EQ(match->RoundState, RoundState::Between);
    EXPECT_EQ(match->TeamScores[1].Round, 1);
    EXPECT_EQ(match->TeamScores[1].Total, 4);

    m_Game.RoundState = 9;
    this->PublishWorld();
    m_Chain.m_Environment.m_MatchService.Update();
    EXPECT_EQ(m_Chain.m_Environment.m_MatchStore.Acquire()->RoundState, RoundState::Unknown);
}