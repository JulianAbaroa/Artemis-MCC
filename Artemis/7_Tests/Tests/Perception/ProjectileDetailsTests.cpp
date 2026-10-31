#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Player.Type;
import Perception.Classifier.Type;
import Perception.Entities.Type;
import Blam.Runtime.Object.Type;
import Perception.Details.Type;
import Tests.Support.Pipeline;
import std;

// W.2 (D254): the projectile details, from an object table filled by hand. The real values come from the same fields W.1 reads.

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Semantic::Type::Function;
    using Common::Team::Type::Team;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::Runtime::Player::Type::Alive::AlivePlayer;
    using Perception::Classifier::Type::Classified;
    using Perception::Classifier::Type::Classifieds;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Biped::Biped;
    using Blam::Runtime::Object::Type::Projectile::Projectile;
    using Perception::Details::Type::ProjectileDetail;
    using Perception::Details::Type::ProjectileDetails;
    using Tests::Support::Pipeline::Chain;

    constexpr std::uint32_t k_InFlight{ 0x1001 };
    constexpr std::uint32_t k_Stuck{ 0x1002 };
    constexpr std::uint32_t k_FromVehicle{ 0x1003 };
    constexpr std::uint32_t k_Biped{ 0x2001 };
    constexpr std::uint32_t k_Victim{ 0x2002 };
    constexpr std::uint32_t k_Vehicle{ 0x2003 };
    constexpr std::uint32_t k_Player{ 0x3001 };

    class ProjectileDetailsTests : public ::testing::Test
    {
    protected:
        auto AddProjectile(std::uint32_t handle, const Projectile& projectile, std::uint32_t parent) -> void
        {
            AliveObject object{};
            object.Handle = handle;
            object.ParentHandle = parent;
            object.Specific = projectile;
            m_Chain.m_Tables.m_ObjectStore.AddObject(handle, object);
        }

        auto Run() -> std::shared_ptr<const ProjectileDetails>
        {
            m_Chain.m_Tables.m_ObjectStore.Publish();
            m_Chain.m_Environment.m_ProjectileDetailsService.Update();
            return m_Chain.m_Environment.m_ProjectileDetailsStore.Acquire();
        }

        Chain m_Chain{};
    };
}

TEST_F(ProjectileDetailsTests, ProjectileInFlightHasOwnersAndNothingItIsStuckTo)
{
    this->AddProjectile(k_InFlight, Projectile{ .OwnerPlayer = k_Player, .OwnerBiped = k_Biped, .ShooterRoot = k_Biped, .ParentNode = -1 }, k_NoHandle);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    ASSERT_EQ(details->size(), 1u);

    const ProjectileDetail& detail{ details->at(k_InFlight) };
    EXPECT_EQ(detail.Handle, k_InFlight);
    EXPECT_EQ(detail.OwnerPlayer, k_Player);
    EXPECT_EQ(detail.OwnerBiped, k_Biped);
    EXPECT_EQ(detail.ShooterRoot, k_Biped);
    EXPECT_EQ(detail.StuckTo, k_NoHandle);
    EXPECT_EQ(detail.StuckNode, -1);
    EXPECT_FALSE(detail.InHand);
}

TEST_F(ProjectileDetailsTests, StuckProjectileKeepsItsVictimAndNode)
{
    this->AddProjectile(k_Stuck, Projectile{ .OwnerPlayer = k_Player, .OwnerBiped = k_Biped, .ShooterRoot = k_Biped, .ParentNode = 7 }, k_Victim);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);

    const ProjectileDetail& detail{ details->at(k_Stuck) };
    EXPECT_EQ(detail.StuckTo, k_Victim);
    EXPECT_EQ(detail.StuckNode, 7);
    EXPECT_FALSE(detail.InHand);
    EXPECT_EQ(detail.OwnerPlayer, k_Player);
}

TEST_F(ProjectileDetailsTests, GrenadeParentedToItsOwnerBipedIsInHandAndNotStuck)
{
    this->AddProjectile(k_Stuck, Projectile{ .OwnerPlayer = k_Player, .OwnerBiped = k_Biped, .ShooterRoot = k_Biped, .ParentNode = 3 }, k_Biped);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);

    const ProjectileDetail& detail{ details->at(k_Stuck) };
    EXPECT_TRUE(detail.InHand);
    EXPECT_EQ(detail.StuckTo, k_NoHandle);
    EXPECT_EQ(detail.StuckNode, -1);
    EXPECT_EQ(detail.OwnerBiped, k_Biped);
}

TEST_F(ProjectileDetailsTests, ProjectileFiredFromAVehicleHasAShooterRootThatIsNotTheBiped)
{
    this->AddProjectile(k_FromVehicle, Projectile{ .OwnerPlayer = k_Player, .OwnerBiped = k_Biped, .ShooterRoot = k_Vehicle, .ParentNode = -1 }, k_NoHandle);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);

    const ProjectileDetail& detail{ details->at(k_FromVehicle) };
    EXPECT_EQ(detail.ShooterRoot, k_Vehicle);
    EXPECT_NE(detail.ShooterRoot, detail.OwnerBiped);
}

TEST_F(ProjectileDetailsTests, ANodeWithoutAParentIsReportedAsNotStuck)
{
    // The engine can leave a node behind after a detach: with no parent the projectile is not stuck whatever the node says.
    this->AddProjectile(k_InFlight, Projectile{ .ParentNode = 4 }, k_NoHandle);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    EXPECT_EQ(details->at(k_InFlight).StuckTo, k_NoHandle);
    EXPECT_EQ(details->at(k_InFlight).StuckNode, -1);
}

TEST_F(ProjectileDetailsTests, OnlyProjectilesGetADetailAndCleanupDropsThem)
{
    AliveObject biped{};
    biped.Handle = k_Biped;
    biped.ParentHandle = k_NoHandle;
    biped.Specific = Biped{};
    m_Chain.m_Tables.m_ObjectStore.AddObject(k_Biped, biped);
    this->AddProjectile(k_InFlight, Projectile{}, k_NoHandle);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    EXPECT_EQ(details->size(), 1u);
    EXPECT_TRUE(details->contains(k_InFlight));

    m_Chain.m_Environment.m_ProjectileDetailsService.Cleanup();
    EXPECT_EQ(m_Chain.m_Environment.m_ProjectileDetailsStore.Acquire(), nullptr);
}

TEST_F(ProjectileDetailsTests, TheEntityOfAProjectileHasTheTeamOfThePlayerThatFiredIt)
{
    // Fired from a vehicle seat (the shooter root is the vehicle): the team still comes from the player.
    this->AddProjectile(k_FromVehicle, Projectile{ .OwnerPlayer = k_Player, .OwnerBiped = k_Biped, .ShooterRoot = k_Vehicle, .ParentNode = -1 }, k_NoHandle);
    this->AddProjectile(k_InFlight, Projectile{ .OwnerPlayer = k_NoHandle, .OwnerBiped = k_NoHandle, .ShooterRoot = k_NoHandle, .ParentNode = -1 }, k_NoHandle);
    m_Chain.m_Tables.m_ObjectStore.Publish();

    m_Chain.m_Tables.m_PlayerStore.AddPlayer(k_Player, AlivePlayer{ .Handle = k_Player, .Team = Team::Blue });
    m_Chain.m_Tables.m_PlayerStore.Publish();

    Classifieds classifieds{};
    classifieds.push_back(Classified{ .Handle = k_FromVehicle, .Kind = ObjectKind::Projectile, .Function = Function::Projectile });
    classifieds.push_back(Classified{ .Handle = k_InFlight, .Kind = ObjectKind::Projectile, .Function = Function::Projectile });
    m_Chain.m_Environment.m_ClassifierStore.Publish(std::move(classifieds));

    m_Chain.m_Environment.m_EntitiesService.Update();
    const auto entities{ m_Chain.m_Environment.m_EntitiesStore.Acquire() };
    ASSERT_NE(entities, nullptr);

    const auto fired{ entities->Index.find(k_FromVehicle) };
    ASSERT_NE(fired, entities->Index.end());
    EXPECT_EQ(entities->Rows[fired->second].Team, Team::Blue);

    // Without a player the team is what it was before: Neutral for a projectile of a map without team.
    const auto noPlayer{ entities->Index.find(k_InFlight) };
    ASSERT_NE(noPlayer, entities->Index.end());
    EXPECT_EQ(entities->Rows[noPlayer->second].Team, Team::Neutral);
}