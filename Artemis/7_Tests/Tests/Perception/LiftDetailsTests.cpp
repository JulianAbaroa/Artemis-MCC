#include <gtest/gtest.h>

import Common.Math.Type;
import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Object.Type;
import Perception.Classifier.Type;
import Perception.Details.Type;
import Tests.Support.Pipeline;
import std;

// A12 (D98, D261): the stages of the phantom types of each lift and the measured exit, from definitions filled by hand.
// The values of forge_halo are in ReplayTests.

namespace
{
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::Function;
    using Blam::MapModel::Definitions::Type::Bloc::Bloc;
    using Blam::MapModel::Definitions::Type::Hlmt::Hlmt;
    using Blam::MapModel::Definitions::Type::Mode::Marker;
    using Blam::MapModel::Definitions::Type::Mode::MarkerGroup;
    using Blam::MapModel::Definitions::Type::Mode::Mode;
    using Blam::MapModel::Definitions::Type::Object::Object;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::MapModel::Definitions::Type::Phmo::PhantomType;
    using Blam::MapModel::Definitions::Type::Phmo::Phmo;
    using Blam::Runtime::Game::Type::Alive::AliveGame;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Perception::Classifier::Type::Classified;
    using Perception::Classifier::Type::Classifieds;
    using Perception::Details::Type::LiftAffects;
    using Perception::Details::Type::LiftDetail;
    using Perception::Details::Type::LiftDetails;
    using Tests::Support::Pipeline::Chain;

    constexpr std::uint32_t k_LiftA{ 0x2001 };
    constexpr std::uint32_t k_LiftB{ 0x2002 };
    constexpr std::uint32_t k_NotALift{ 0x2003 };
    constexpr std::uint32_t k_Biped{ 0x3001 };
    constexpr std::uint32_t k_OtherBiped{ 0x3002 };

    constexpr std::uint32_t k_MarkerName{ 0xED };
    constexpr std::uint32_t k_OrphanMarkerName{ 0x77 };

    // Flags measured in forge_halo: ff_man_cannon_forge (all but the doors' bits) and heh_gravlift (no vehicles).
    constexpr std::uint32_t k_CannonFlags{ 0x00001B00 };
    constexpr std::uint32_t k_GravLiftFlags{ 0x00007A40 };
    // Ignores players (bit 3) and bipeds (bit 5): the stage of ff_grav_lift for the items.
    constexpr std::uint32_t k_ItemsOnlyFlags{ 0x00000028 };

    constexpr std::uint8_t k_PlayersBipedsVehiclesItems{ 15 };
    constexpr std::uint8_t k_PlayersBipedsItems{ 11 };

    const std::string k_LiftTag{ "test\\lift" };
    const std::string k_NoModelTag{ "test\\no_model" };
    const std::string k_LiftModel{ "test\\lift_model" };
    const std::string k_LiftPhysics{ "test\\lift_physics" };
    const std::string k_LiftRender{ "test\\lift_render" };

    // The measured direction of the second stage of ff_man_cannon_forge in the space of the object.
    constexpr float k_LocalX{ -0.82f };

    class LiftDetailsTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            this->FillDefinitions();
            m_Chain.m_Tables.m_GameStore.Publish(AliveGame{});
        }

        // The tag has three phantom types: all kinds from a marker, the gravity lift flags without marker, and a stage for the items only.
        auto FillDefinitions() -> void
        {
            auto& definitions{ m_Chain.m_Resolved.m_DefinitionsStore };
            definitions.m_Bloc.Add(k_LiftTag, Bloc{ .Base = Object{ .ModelTagName = k_LiftModel } });
            definitions.m_Hlmt.Add(k_LiftModel, Hlmt{ .RenderModelTagName = k_LiftRender, .PhysicsModelTagName = k_LiftPhysics });

            const float angle{ std::atan2(-std::sqrt(1.0f - k_LocalX * k_LocalX), k_LocalX) };
            Marker marker{ .NodeIndex = 0, .Translation = Vec3{ 0.5f, 0.25f, 2.0f },
                .Rotation = { 0.0f, std::sin(angle / 2.0f), 0.0f, std::cos(angle / 2.0f) } };
            definitions.m_Mode.Add(k_LiftRender, Mode{ .MarkerGroups = { MarkerGroup{ .NameId = k_MarkerName, .Markers = { marker } } } });

            definitions.m_Phmo.Add(k_LiftPhysics, Phmo{ .PhantomTypes = {
                PhantomType{ .Flags = k_CannonFlags, .MarkerName = k_MarkerName, .AxisAcceleration = 25.0f, .AxisMaximumVelocity = 20.0f,
                    .DirectionAcceleration = 11.0f, .DirectionMaximumVelocity = 7.5f },
                PhantomType{ .Flags = k_ItemsOnlyFlags, .MarkerName = k_MarkerName, .DirectionAcceleration = 5.0f, .DirectionMaximumVelocity = 2.0f },
                PhantomType{ .Flags = k_GravLiftFlags, .MarkerName = 0, .DirectionAcceleration = 10.0f, .DirectionMaximumVelocity = 4.0f },
                PhantomType{ .Flags = k_CannonFlags, .MarkerName = k_OrphanMarkerName, .DirectionAcceleration = 1.0f, .DirectionMaximumVelocity = 1.0f } } });

            definitions.m_Bloc.Freeze();
            definitions.m_Hlmt.Freeze();
            definitions.m_Mode.Freeze();
            definitions.m_Phmo.Freeze();
        }

        // A lift with Forward (0, 1, 0) and Up (0, 0, 1), so its Left is (-1, 0, 0).
        auto AddLift(std::uint32_t handle, const std::string& tagName, const Vec3& origin, Function function = Function::Lift) -> void
        {
            AliveObject object{};
            object.Handle = handle;
            object.TagName = tagName;
            object.WorldOrigin = origin;
            object.Origin = origin;
            object.Forward = Vec3{ 0.0f, 1.0f, 0.0f };
            object.Up = Vec3{ 0.0f, 0.0f, 1.0f };
            m_Chain.m_Tables.m_ObjectStore.AddObject(handle, object);
            m_Classifieds.push_back(Classified{ .Handle = handle, .Kind = ObjectKind::Crate, .Function = function });
        }

        auto AddBiped(std::uint32_t handle, const Vec3& origin) -> void
        {
            AliveObject object{};
            object.Handle = handle;
            object.WorldOrigin = origin;
            object.Origin = origin;
            object.Profile.ObjectKind = ObjectKind::Biped;
            m_Chain.m_Tables.m_ObjectStore.AddObject(handle, object);
        }

        // Sets the phantom bit and the velocity of a biped in the table, as the engine does between two ticks.
        auto Set(std::uint32_t handle, bool isInPhantom, const Vec3& velocity = Vec3{}) -> void
        {
            m_Chain.m_Tables.m_ObjectStore.UpdateObjects([&](std::uint32_t objectHandle, AliveObject& object)
            {
                if (objectHandle != handle) return;

                object.IsInPhantom = isInPhantom;
                object.LinearVelocity = velocity;
            });
        }

        auto Run(std::uint32_t gameTick = 0) -> std::shared_ptr<const LiftDetails>
        {
            m_Chain.m_Tables.m_ObjectStore.Publish();
            m_Chain.m_Environment.m_ClassifierStore.Publish(Classifieds{ m_Classifieds });
            m_Chain.m_Tables.m_GameStore.Publish(AliveGame{ .EngineTick = gameTick });
            m_Chain.m_Environment.m_LiftDetailsService.Update();
            return m_Chain.m_Environment.m_LiftDetailsStore.Acquire();
        }

        Chain m_Chain{};
        Classifieds m_Classifieds{};
    };

    constexpr auto Bits(LiftAffects a, LiftAffects b) -> std::uint8_t
    {
        return static_cast<std::uint8_t>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
    }
}

TEST_F(LiftDetailsTests, StageTakesTheMarkerRotatedToTheWorldOfTheLift)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{ 100.0f, 200.0f, 10.0f });

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    const LiftDetail& lift{ details->at(k_LiftA) };
    ASSERT_GE(lift.Stages.size(), 1u);

    // +X of the marker is (-0.82, 0, z) in the object, with Forward (0, 1, 0) it is (0, -0.82, z) in the world.
    const auto& stage{ lift.Stages[0] };
    EXPECT_NEAR(stage.Direction.X, 0.0f, 1e-5f);
    EXPECT_NEAR(stage.Direction.Y, k_LocalX, 1e-5f);
    EXPECT_NEAR(stage.Direction.Z, std::sqrt(1.0f - k_LocalX * k_LocalX), 1e-5f);

    // Translation (0.5, 0.25, 2) = 0.5 Forward + 0.25 Left + 2 Up, with Left (-1, 0, 0).
    EXPECT_NEAR(stage.Position.X, 99.75f, 1e-5f);
    EXPECT_NEAR(stage.Position.Y, 200.5f, 1e-5f);
    EXPECT_NEAR(stage.Position.Z, 12.0f, 1e-5f);

    EXPECT_FLOAT_EQ(stage.Acceleration, 11.0f);
    EXPECT_FLOAT_EQ(stage.MaximumVelocity, 7.5f);
    EXPECT_FLOAT_EQ(stage.AxisAcceleration, 25.0f);
    EXPECT_FLOAT_EQ(stage.AxisMaximumVelocity, 20.0f);
}

TEST_F(LiftDetailsTests, StageWithoutMarkerPointsUpFromTheOriginAndOneWithoutGroupToo)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{ 1.0f, 2.0f, 3.0f });

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    const LiftDetail& lift{ details->at(k_LiftA) };
    ASSERT_EQ(lift.Stages.size(), 3u);

    for (const std::size_t index : { std::size_t{ 1 }, std::size_t{ 2 } })
    {
        EXPECT_EQ(lift.Stages[index].Position.X, 1.0f);
        EXPECT_EQ(lift.Stages[index].Position.Y, 2.0f);
        EXPECT_EQ(lift.Stages[index].Position.Z, 3.0f);
        EXPECT_EQ(lift.Stages[index].Direction.X, 0.0f);
        EXPECT_EQ(lift.Stages[index].Direction.Y, 0.0f);
        EXPECT_EQ(lift.Stages[index].Direction.Z, 1.0f);
    }
}

TEST_F(LiftDetailsTests, StageThatIgnoresPlayersAndBipedsIsDroppedAndTheOthersKeepTheOrderOfTheTag)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{});

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    const LiftDetail& lift{ details->at(k_LiftA) };

    // Types 0, 2 and 3 of the tag: the items-only type 1 does not count.
    ASSERT_EQ(lift.Stages.size(), 3u);
    EXPECT_FLOAT_EQ(lift.Stages[0].Acceleration, 11.0f);
    EXPECT_FLOAT_EQ(lift.Stages[1].Acceleration, 10.0f);
    EXPECT_FLOAT_EQ(lift.Stages[2].Acceleration, 1.0f);
}

TEST_F(LiftDetailsTests, AffectsComeFromTheMeasuredFlags)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{});

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    const LiftDetail& lift{ details->at(k_LiftA) };
    ASSERT_EQ(lift.Stages.size(), 3u);
    EXPECT_EQ(lift.Stages[0].Affects, k_PlayersBipedsVehiclesItems);
    EXPECT_EQ(lift.Stages[1].Affects, k_PlayersBipedsItems);
    EXPECT_EQ(lift.Stages[0].Affects, Bits(LiftAffects::Players, LiftAffects::Bipeds) | static_cast<std::uint8_t>(LiftAffects::Vehicles) |
        static_cast<std::uint8_t>(LiftAffects::Items));
    EXPECT_EQ(lift.Stages[1].Affects & static_cast<std::uint8_t>(LiftAffects::Vehicles), 0);
}

TEST_F(LiftDetailsTests, LiftWithoutDefinitionsOrTagHasNoStagesButIsPublishedAndOthersHaveNoDetail)
{
    this->AddLift(k_LiftA, k_NoModelTag, Vec3{});
    this->AddLift(k_NotALift, k_LiftTag, Vec3{}, Function::Unknown);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    ASSERT_EQ(details->size(), 1u);
    EXPECT_TRUE(details->at(k_LiftA).Stages.empty());
    EXPECT_FALSE(details->at(k_LiftA).HasExit);

    Chain bare{};
    AliveObject object{};
    object.Handle = k_LiftA;
    object.TagName = k_LiftTag;
    bare.m_Tables.m_ObjectStore.AddObject(k_LiftA, object);
    bare.m_Tables.m_ObjectStore.Publish();
    Classifieds bareClassifieds{};
    bareClassifieds.push_back(Classified{ .Handle = k_LiftA, .Kind = ObjectKind::Crate, .Function = Function::Lift });
    bare.m_Environment.m_ClassifierStore.Publish(std::move(bareClassifieds));
    bare.m_Environment.m_LiftDetailsService.Update();
    const auto bareDetails{ bare.m_Environment.m_LiftDetailsStore.Acquire() };
    ASSERT_NE(bareDetails, nullptr);
    EXPECT_TRUE(bareDetails->at(k_LiftA).Stages.empty());
}

TEST_F(LiftDetailsTests, BipedThatLeavesThePhantomGivesTheExitToTheNearestLiftOnly)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{ 0.0f, 0.0f, 0.0f });
    this->AddLift(k_LiftB, k_LiftTag, Vec3{ 4.2f, 0.0f, 0.0f });
    this->AddBiped(k_Biped, Vec3{ 0.8f, 0.0f, 0.0f });

    ASSERT_NE(this->Run(100), nullptr);
    this->Set(k_Biped, true);
    auto details{ this->Run(101) };
    EXPECT_FALSE(details->at(k_LiftA).HasExit);

    this->Set(k_Biped, true, Vec3{ 0.25f, -6.0f, 4.0f });
    details = this->Run(150);
    EXPECT_FALSE(details->at(k_LiftA).HasExit);

    this->Set(k_Biped, false, Vec3{ 0.252f, -6.162f, 4.114f });
    details = this->Run(151);
    const LiftDetail& lift{ details->at(k_LiftA) };
    EXPECT_TRUE(lift.HasExit);
    EXPECT_EQ(lift.ExitTick, 151u);
    EXPECT_EQ(lift.ExitBiped, k_Biped);
    EXPECT_FLOAT_EQ(lift.ExitVelocity.X, 0.252f);
    EXPECT_FLOAT_EQ(lift.ExitVelocity.Y, -6.162f);
    EXPECT_FLOAT_EQ(lift.ExitVelocity.Z, 4.114f);
    EXPECT_FALSE(details->at(k_LiftB).HasExit);

    // The exit stays published until another one replaces it.
    details = this->Run(152);
    EXPECT_TRUE(details->at(k_LiftA).HasExit);
    EXPECT_EQ(details->at(k_LiftA).ExitTick, 151u);
}

TEST_F(LiftDetailsTests, BipedThatEntersFarFromEveryLiftHasNoOwner)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{});
    this->AddBiped(k_Biped, Vec3{ 3.5f, 0.0f, 0.0f });

    this->Run(1);
    this->Set(k_Biped, true);
    this->Run(2);
    this->Set(k_Biped, false, Vec3{ 0.0f, 0.0f, 5.0f });
    const auto details{ this->Run(3) };
    EXPECT_FALSE(details->at(k_LiftA).HasExit);
}

TEST_F(LiftDetailsTests, BipedRemovedInsideThePhantomLeavesNoExit)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{});
    this->AddBiped(k_Biped, Vec3{ 0.5f, 0.0f, 0.0f });

    this->Run(1);
    this->Set(k_Biped, true);
    this->Run(2);
    m_Chain.m_Tables.m_ObjectStore.RemoveObject(k_Biped);
    EXPECT_FALSE(this->Run(3)->at(k_LiftA).HasExit);

    // The same handle back and out of the volume is a new biped: it was forgotten, so nothing leaves.
    this->AddBiped(k_Biped, Vec3{ 0.5f, 0.0f, 0.0f });
    EXPECT_FALSE(this->Run(4)->at(k_LiftA).HasExit);
}

TEST_F(LiftDetailsTests, TwoExitsLeaveTheLastOne)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{});
    this->AddBiped(k_Biped, Vec3{ 0.5f, 0.0f, 0.0f });
    this->AddBiped(k_OtherBiped, Vec3{ -0.5f, 0.0f, 0.0f });

    this->Run(1);
    this->Set(k_Biped, true);
    this->Run(2);
    this->Set(k_Biped, false, Vec3{ 0.0f, 0.0f, 3.0f });
    EXPECT_EQ(this->Run(3)->at(k_LiftA).ExitBiped, k_Biped);

    this->Set(k_OtherBiped, true);
    this->Run(4);
    this->Set(k_OtherBiped, false, Vec3{ 1.0f, 0.0f, 8.0f });
    const auto details{ this->Run(5) };
    const LiftDetail& lift{ details->at(k_LiftA) };
    EXPECT_EQ(lift.ExitBiped, k_OtherBiped);
    EXPECT_EQ(lift.ExitTick, 5u);
    EXPECT_FLOAT_EQ(lift.ExitVelocity.Z, 8.0f);
}

TEST_F(LiftDetailsTests, ExitOfALiftThatDisappearsIsForgottenAndCleanupEmptiesEverything)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{});
    this->AddBiped(k_Biped, Vec3{ 0.5f, 0.0f, 0.0f });

    this->Run(1);
    this->Set(k_Biped, true);
    this->Run(2);
    this->Set(k_Biped, false, Vec3{ 0.0f, 0.0f, 3.0f });
    EXPECT_TRUE(this->Run(3)->at(k_LiftA).HasExit);

    m_Chain.m_Tables.m_ObjectStore.RemoveObject(k_LiftA);
    m_Classifieds.clear();
    EXPECT_TRUE(this->Run(4)->empty());

    // The lift returns with the same handle: its exit is gone.
    this->AddLift(k_LiftA, k_LiftTag, Vec3{});
    EXPECT_FALSE(this->Run(5)->at(k_LiftA).HasExit);

    // Cleanup: an exit that was there goes with the memory.
    this->Set(k_Biped, true);
    this->Run(6);
    this->Set(k_Biped, false, Vec3{ 0.0f, 0.0f, 3.0f });
    EXPECT_TRUE(this->Run(7)->at(k_LiftA).HasExit);
    m_Chain.m_Environment.m_LiftDetailsService.Cleanup();
    EXPECT_EQ(m_Chain.m_Environment.m_LiftDetailsStore.Acquire(), nullptr);
    EXPECT_FALSE(this->Run(8)->at(k_LiftA).HasExit);
}

// Q101.A: updates that skip engine ticks (the sparse mode). The exit is the first sample outside the volume.
TEST_F(LiftDetailsTests, ABipedThatEntersAndLeavesBetweenTwoSamplesIsDatedInTheSampleOfTheExit)
{
    this->AddLift(k_LiftA, k_LiftTag, Vec3{ 0.0f, 0.0f, 0.0f });
    this->AddBiped(k_Biped, Vec3{ 0.8f, 0.0f, 0.0f });
    ASSERT_NE(this->Run(100), nullptr);

    // Inside at the sample 104 and 108, out at 112: the exit is at 112 with the velocity of that sample.
    this->Set(k_Biped, true, Vec3{ 0.0f, 0.0f, 2.0f });
    EXPECT_FALSE(this->Run(104)->at(k_LiftA).HasExit);
    this->Set(k_Biped, true, Vec3{ 0.0f, 0.0f, 3.0f });
    EXPECT_FALSE(this->Run(108)->at(k_LiftA).HasExit);
    this->Set(k_Biped, false, Vec3{ 0.0f, 0.0f, 4.5f });
    const auto details{ this->Run(112) };
    EXPECT_TRUE(details->at(k_LiftA).HasExit);
    EXPECT_EQ(details->at(k_LiftA).ExitTick, 112u);
    EXPECT_FLOAT_EQ(details->at(k_LiftA).ExitVelocity.Z, 4.5f);

    // A biped that is inside and out between two samples is never seen inside: no exit (declared tolerance).
    this->AddBiped(k_OtherBiped, Vec3{ 0.5f, 0.0f, 0.0f });
    this->Run(116);
    this->Set(k_OtherBiped, false, Vec3{ 0.0f, 0.0f, 9.0f });
    const auto after{ this->Run(120) };
    EXPECT_EQ(after->at(k_LiftA).ExitBiped, k_Biped);
    EXPECT_EQ(after->at(k_LiftA).ExitTick, 112u);
}