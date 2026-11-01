#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Blam.Runtime.Object.Type;
import Blam.Runtime.Player.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
import Tests.Support.Pipeline;
import std;

// A3c (D260): the ground and the crouch of the body of a player, from sequences of ticks made by hand. The real values are checked by
// ReplayTests.SueloGroundAndCrouchFollowTheRecordedFilm.

namespace
{
    using Common::Math::Type::Vec3;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Biped::Biped;
    using Blam::Runtime::Player::Type::Alive::AlivePlayer;
    using Blam::Runtime::Player::Type::Alive::ConnectionState;
    using Perception::Entities::Type::Entities;
    using Perception::Entities::Type::Entity;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::GroundKind;
    using Tests::Support::Pipeline::Chain;

    using Blam::Runtime::Object::Type::Biped::k_NoContact;
    using Common::Engine::Type::k_NoHandle;

    constexpr std::uint32_t k_BipedHandle{ 0xE5B30344 };
    constexpr std::uint32_t k_PlayerHandle{ 0xE5A10000 };
    constexpr std::uint32_t k_BlockHandle{ 0xE4030194 };
    constexpr std::uint32_t k_OtherHandle{ 0xE33B00CC };

    // What the engine reads in one tick for the biped.
    struct Frame
    {
        std::uint16_t Material{ k_NoContact };
        bool OnStructure{ false };
        std::uint32_t GroundHandle{ k_NoHandle };
        std::uint32_t Parent{ k_NoHandle };
        Vec3 Velocity{};
        float Crouch{};
        // Whether the object under the feet is in the object table, and how fast it moves.
        bool IsGroundInTable{ true };
        Vec3 GroundVelocity{};
    };

    // Contact with an object, as the engine reports it.
    auto OnBlock(std::uint32_t handle = k_BlockHandle) -> Frame
    {
        return Frame{ .Material = 121, .GroundHandle = handle };
    }

    // No contact, with a vertical speed.
    auto InAir(float z) -> Frame
    {
        return Frame{ .Velocity = { 0.0f, 0.0f, z } };
    }

    class PlayerGroundTests : public ::testing::Test
    {
    protected:
        // Publishes the tables of a tick with the biped as the frame says, runs the players service and returns the body.
        auto Step(const Frame& frame) -> Body
        {
            auto& objects{ m_Chain.m_Tables.m_ObjectStore };
            Biped biped{};
            biped.ContactMaterial = frame.Material;
            biped.OnStructure = frame.OnStructure;
            biped.GroundObjectHandle = frame.GroundHandle;
            biped.CrouchFraction = frame.Crouch;

            AliveObject object{};
            object.Handle = k_BipedHandle;
            object.ParentHandle = frame.Parent;
            object.LinearVelocity = frame.Velocity;
            object.Specific = biped;
            objects.RemoveObject(k_BipedHandle);
            objects.AddObject(k_BipedHandle, object);

            objects.RemoveObject(k_BlockHandle);
            if (frame.IsGroundInTable)
            {
                AliveObject block{};
                block.Handle = k_BlockHandle;
                block.ParentHandle = k_NoHandle;
                block.LinearVelocity = frame.GroundVelocity;
                objects.AddObject(k_BlockHandle, block);
            }
            objects.Publish();

            AlivePlayer player{};
            player.Handle = k_PlayerHandle;
            player.ConnectionState = ConnectionState::Connected;
            player.AliveBipedHandle = k_BipedHandle;
            player.CurrentBipedHandle = k_BipedHandle;
            m_Chain.m_Tables.m_PlayerStore.AddPlayer(k_PlayerHandle, player);
            m_Chain.m_Tables.m_PlayerStore.Publish();

            Entities entities{};
            entities.Rows.push_back(Entity{ .Handle = k_BipedHandle });
            entities.Index.emplace(k_BipedHandle, 0);
            m_Chain.m_Environment.m_EntitiesStore.Publish(std::move(entities));

            m_Chain.m_Environment.m_PlayersService.Update();
            const auto players{ m_Chain.m_Environment.m_PlayersStore.Acquire() };
            return players->Rows.at(0).Body.value();
        }

        Chain m_Chain{};
    };
}

TEST_F(PlayerGroundTests, AJumpIsNotGroundedFromTheFirstTickWithoutContact)
{
    this->Step(Frame{ .Material = 5, .OnStructure = true });

    const Body first{ this->Step(InAir(2.3f)) };
    EXPECT_FALSE(first.IsGrounded);
    EXPECT_EQ(first.AirborneTicks, 1u);
    EXPECT_EQ(first.Ground, GroundKind::Air);

    const Body second{ this->Step(InAir(2.0f)) };
    EXPECT_FALSE(second.IsGrounded);
    EXPECT_EQ(second.AirborneTicks, 2u);
    EXPECT_EQ(second.Ground, GroundKind::Air);
}

TEST_F(PlayerGroundTests, AFlickerOfOneOrTwoTicksKeepsTheLastContact)
{
    const Body contact{ this->Step(OnBlock()) };
    EXPECT_EQ(contact.Ground, GroundKind::Object);
    EXPECT_TRUE(contact.IsGrounded);

    const Body first{ this->Step(InAir(-0.3f)) };
    EXPECT_TRUE(first.IsGrounded);
    EXPECT_EQ(first.Ground, GroundKind::Object);
    EXPECT_EQ(first.GroundObject, k_BlockHandle);
    EXPECT_EQ(first.AirborneTicks, 1u);

    const Body second{ this->Step(InAir(-0.3f)) };
    EXPECT_TRUE(second.IsGrounded);
    EXPECT_EQ(second.Ground, GroundKind::Object);
    EXPECT_EQ(second.GroundObject, k_BlockHandle);
    EXPECT_EQ(second.AirborneTicks, 2u);

    const Body third{ this->Step(InAir(-0.3f)) };
    EXPECT_FALSE(third.IsGrounded);
    EXPECT_EQ(third.Ground, GroundKind::Air);
    EXPECT_EQ(third.GroundObject, k_NoHandle);
    EXPECT_EQ(third.AirborneTicks, 3u);

    const Body landed{ this->Step(OnBlock()) };
    EXPECT_EQ(landed.AirborneTicks, 0u);
    EXPECT_TRUE(landed.IsGrounded);
}

TEST_F(PlayerGroundTests, StandingStillOnAnObjectKeepsTheLastValidGroundObject)
{
    this->Step(OnBlock());
    for (int tick{ 0 }; tick < 300; ++tick)
    {
        const Body body{ this->Step(OnBlock(k_NoHandle)) };
        ASSERT_EQ(body.Ground, GroundKind::Object);
        ASSERT_EQ(body.GroundObject, k_BlockHandle);
    }

    // A cut of 3 ticks or more forgets it.
    for (int tick{ 0 }; tick < 3; ++tick) this->Step(InAir(-0.3f));
    const Body again{ this->Step(OnBlock(k_NoHandle)) };
    EXPECT_EQ(again.Ground, GroundKind::Object);
    EXPECT_EQ(again.GroundObject, k_NoHandle);
}

TEST_F(PlayerGroundTests, AShortCutDoesNotForgetTheGroundObject)
{
    this->Step(OnBlock());
    this->Step(InAir(-0.3f));
    this->Step(InAir(-0.3f));
    const Body again{ this->Step(OnBlock(k_NoHandle)) };
    EXPECT_EQ(again.GroundObject, k_BlockHandle);
}

TEST_F(PlayerGroundTests, TheBspIsStaticWithoutGroundObject)
{
    const Body body{ this->Step(Frame{ .Material = 5, .OnStructure = true }) };
    EXPECT_EQ(body.Ground, GroundKind::Static);
    EXPECT_TRUE(body.IsGrounded);
    EXPECT_EQ(body.GroundObject, k_NoHandle);
    EXPECT_EQ(body.AirborneTicks, 0u);
}

TEST_F(PlayerGroundTests, AParentedBipedIsAttachedAndNeverGrounded)
{
    const Body body{ this->Step(Frame{ .Parent = k_OtherHandle }) };
    EXPECT_EQ(body.Ground, GroundKind::Attached);
    EXPECT_FALSE(body.IsGrounded);
    EXPECT_EQ(body.AirborneTicks, 0u);
    EXPECT_EQ(body.GroundObject, k_NoHandle);

    // Contact is ignored while attached, and leaving the seat in the air does not hold anything.
    const Body withContact{ this->Step(Frame{ .Material = 5, .OnStructure = true, .Parent = k_OtherHandle }) };
    EXPECT_EQ(withContact.Ground, GroundKind::Attached);
    EXPECT_FALSE(withContact.IsGrounded);

    const Body left{ this->Step(InAir(0.0f)) };
    EXPECT_EQ(left.Ground, GroundKind::Air);
    EXPECT_FALSE(left.IsGrounded);
    EXPECT_EQ(left.AirborneTicks, 1u);
}

TEST_F(PlayerGroundTests, ANewBipedWithoutContactStartsInTheAir)
{
    const Body body{ this->Step(InAir(0.0f)) };
    EXPECT_EQ(body.Ground, GroundKind::Air);
    EXPECT_FALSE(body.IsGrounded);
    EXPECT_EQ(body.AirborneTicks, 1u);
}

TEST_F(PlayerGroundTests, OwnVelocityIsRelativeToTheObjectUnderTheFeet)
{
    Frame onBlock{ OnBlock() };
    onBlock.Velocity = { 1.0f, 0.0f, 0.0f };
    onBlock.GroundVelocity = { 1.0f, 0.0f, 0.0f };
    const Body moving{ this->Step(onBlock) };
    EXPECT_FLOAT_EQ(moving.OwnVelocity.X, 0.0f);
    EXPECT_FLOAT_EQ(moving.OwnVelocity.Y, 0.0f);
    EXPECT_FLOAT_EQ(moving.OwnVelocity.Z, 0.0f);

    // An object that is not in the table of the tick gives the velocity of the biped itself.
    onBlock.IsGroundInTable = false;
    const Body missing{ this->Step(onBlock) };
    EXPECT_FLOAT_EQ(missing.OwnVelocity.X, 1.0f);
}

TEST_F(PlayerGroundTests, AJumpOffAMovingObjectUsesTheVelocityRelativeToIt)
{
    Frame onBlock{ OnBlock() };
    onBlock.GroundVelocity = { 0.0f, 0.0f, 3.0f };
    this->Step(onBlock);

    // The biped rises with the object: no jump, so the contact is held.
    Frame rising{ InAir(3.0f) };
    rising.GroundVelocity = { 0.0f, 0.0f, 3.0f };
    const Body body{ this->Step(rising) };
    EXPECT_TRUE(body.IsGrounded);
    EXPECT_EQ(body.Ground, GroundKind::Object);
    EXPECT_NEAR(body.OwnVelocity.Z, 0.0f, 1e-6f);
}

TEST_F(PlayerGroundTests, CrouchingStartsAtHalf)
{
    EXPECT_FALSE(this->Step(Frame{ .Crouch = 0.4999f }).IsCrouching);
    const Body half{ this->Step(Frame{ .Crouch = 0.5f }) };
    EXPECT_TRUE(half.IsCrouching);
    EXPECT_FLOAT_EQ(half.CrouchFraction, 0.5f);
}

TEST_F(PlayerGroundTests, CleanupEmptiesTheMemory)
{
    this->Step(OnBlock());
    m_Chain.m_Environment.m_PlayersService.Cleanup();

    // A biped that touches an object without a valid handle after a cleanup has no history.
    const Body body{ this->Step(OnBlock(k_NoHandle)) };
    EXPECT_EQ(body.GroundObject, k_NoHandle);

    m_Chain.m_Environment.m_PlayersService.Cleanup();
    const Body flicker{ this->Step(InAir(-0.3f)) };
    EXPECT_FALSE(flicker.IsGrounded);
    EXPECT_EQ(flicker.AirborneTicks, 1u);
}