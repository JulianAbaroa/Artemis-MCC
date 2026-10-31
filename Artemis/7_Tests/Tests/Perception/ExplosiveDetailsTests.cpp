#include <gtest/gtest.h>

import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Object.Type;
import Perception.Classifier.Type;
import Perception.Details.Type;
import Tests.Support.Pipeline;
import std;

// B4x (D259): the explosion of each explosive, from definitions filled by hand. The values of forge_halo are in MapModelTests.

namespace
{
    using Common::Semantic::Type::Function;
    using Blam::MapModel::Definitions::Type::Bloc::Bloc;
    using Blam::MapModel::Definitions::Type::Hlmt::DamageSection;
    using Blam::MapModel::Definitions::Type::Hlmt::Hlmt;
    using Blam::MapModel::Definitions::Type::Hlmt::InstantResponse;
    using Blam::MapModel::Definitions::Type::Jpt::Jpt;
    using Blam::MapModel::Definitions::Type::Object::Object;
    using Blam::MapModel::Definitions::Type::Object::ObjectKind;
    using Blam::MapModel::Definitions::Type::Scen::Scen;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Perception::Classifier::Type::Classified;
    using Perception::Classifier::Type::Classifieds;
    using Perception::Details::Type::ExplosiveDetail;
    using Perception::Details::Type::ExplosiveDetails;
    using Tests::Support::Pipeline::Chain;

    constexpr std::uint32_t k_Coil{ 0x1001 };
    constexpr std::uint32_t k_Tank{ 0x1002 };
    constexpr std::uint32_t k_Unknown{ 0x1003 };
    constexpr std::uint32_t k_Crate{ 0x1004 };

    const std::string k_CoilTag{ "test\\coil" };
    const std::string k_TankTag{ "test\\tank" };
    const std::string k_UnknownTag{ "test\\unknown" };
    const std::string k_CoilModel{ "test\\coil_model" };
    const std::string k_TankModel{ "test\\tank_model" };
    const std::string k_CoilBlast{ "test\\coil_blast" };
    const std::string k_TankLeak{ "test\\tank_leak" };
    const std::string k_TankBlast{ "test\\tank_blast" };

    class ExplosiveDetailsTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            auto& definitions{ m_Chain.m_Resolved.m_DefinitionsStore };
            definitions.m_Bloc.Add(k_CoilTag, Bloc{ .Base = Object{ .ModelTagName = k_CoilModel } });
            definitions.m_Scen.Add(k_TankTag, Scen{ .Base = Object{ .ModelTagName = k_TankModel } });

            // The coil fires the same blast at 0.99 and at 0, like forge_halo.
            definitions.m_Hlmt.Add(k_CoilModel, Hlmt{ .DamageSections = { DamageSection{ .InstantResponses = {
                InstantResponse{ .DamageThreshold = 0.99f, .TransitionDamageEffectTagName = k_CoilBlast },
                InstantResponse{ .DamageThreshold = 0.0f, .TransitionDamageEffectTagName = k_CoilBlast } } } } });
            // The tank: a response without effect, another effect at half health and the blast when destroyed, in two sections.
            definitions.m_Hlmt.Add(k_TankModel, Hlmt{ .DamageSections = {
                DamageSection{ .InstantResponses = { InstantResponse{ .DamageThreshold = 0.75f },
                    InstantResponse{ .DamageThreshold = 0.5f, .TransitionDamageEffectTagName = k_TankLeak } } },
                DamageSection{ .InstantResponses = { InstantResponse{ .DamageThreshold = 0.0f, .TransitionDamageEffectTagName = k_TankBlast } } } } });

            definitions.m_Jpt.Add(k_CoilBlast, Jpt{ .RadiusMin = 0.8f, .RadiusMax = 2.0f, .DamageUpperBoundMax = 200.0f });
            definitions.m_Jpt.Add(k_TankLeak, Jpt{ .RadiusMin = 0.1f, .RadiusMax = 0.2f, .DamageUpperBoundMax = 1.0f });
            definitions.m_Jpt.Add(k_TankBlast, Jpt{ .RadiusMin = 1.0f, .RadiusMax = 3.0f, .DamageUpperBoundMin = 50.0f, .DamageUpperBoundMax = 150.0f });

            definitions.m_Bloc.Freeze();
            definitions.m_Scen.Freeze();
            definitions.m_Hlmt.Freeze();
            definitions.m_Jpt.Freeze();
        }

        auto Add(std::uint32_t handle, const std::string& tagName, Function function) -> void
        {
            AliveObject object{};
            object.Handle = handle;
            object.TagName = tagName;
            m_Chain.m_Tables.m_ObjectStore.AddObject(handle, object);
            m_Classifieds.push_back(Classified{ .Handle = handle, .Kind = ObjectKind::Crate, .Function = function });
        }

        auto Run() -> std::shared_ptr<const ExplosiveDetails>
        {
            m_Chain.m_Tables.m_ObjectStore.Publish();
            m_Chain.m_Environment.m_ClassifierStore.Publish(Classifieds{ m_Classifieds });
            m_Chain.m_Environment.m_ExplosiveDetailsService.Update();
            return m_Chain.m_Environment.m_ExplosiveDetailsStore.Acquire();
        }

        Chain m_Chain{};
        Classifieds m_Classifieds{};
    };
}

TEST_F(ExplosiveDetailsTests, CrateTakesTheBlastOfItsModel)
{
    this->Add(k_Coil, k_CoilTag, Function::Explosive);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    const ExplosiveDetail& coil{ details->at(k_Coil) };
    EXPECT_EQ(coil.Handle, k_Coil);
    EXPECT_FLOAT_EQ(coil.RadiusMin, 0.8f);
    EXPECT_FLOAT_EQ(coil.RadiusMax, 2.0f);
    EXPECT_FLOAT_EQ(coil.Damage, 200.0f);
}

TEST_F(ExplosiveDetailsTests, SceneryTakesTheEffectOfTheLowestThresholdAcrossSections)
{
    this->Add(k_Tank, k_TankTag, Function::Explosive);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    const ExplosiveDetail& tank{ details->at(k_Tank) };
    EXPECT_FLOAT_EQ(tank.RadiusMin, 1.0f);
    EXPECT_FLOAT_EQ(tank.RadiusMax, 3.0f);
    // The upper bound of the damage, not its minimum.
    EXPECT_FLOAT_EQ(tank.Damage, 150.0f);
}

TEST_F(ExplosiveDetailsTests, ExplosiveWithoutTagsHasAZeroExplosionAndOthersHaveNoDetail)
{
    this->Add(k_Unknown, k_UnknownTag, Function::Explosive);
    this->Add(k_Crate, k_CoilTag, Function::Unknown);

    const auto details{ this->Run() };
    ASSERT_NE(details, nullptr);
    ASSERT_EQ(details->size(), 1u);
    const ExplosiveDetail& unknown{ details->at(k_Unknown) };
    EXPECT_EQ(unknown.RadiusMin, 0.0f);
    EXPECT_EQ(unknown.RadiusMax, 0.0f);
    EXPECT_EQ(unknown.Damage, 0.0f);

    m_Chain.m_Environment.m_ExplosiveDetailsService.Cleanup();
    EXPECT_EQ(m_Chain.m_Environment.m_ExplosiveDetailsStore.Acquire(), nullptr);
}