#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Semantic.Type;
import Blam.MapModel.Definitions.Type;
import Blam.Runtime.Game.Type;
import Blam.Runtime.Object.Type;
import Perception.Details.Type;
import Tests.Support.Pipeline;
import std;

// W.3 (D254): the weapon details, from an object table and a tag filled by hand. The real values are checked by
// ReplayTests.ArmasAWeaponsAndProjectilesFollowTheRecordedFilm.

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Semantic::Type::ChargeState;
    using Blam::MapModel::Definitions::Type::Weap::Magazine;
    using Blam::MapModel::Definitions::Type::Weap::Trigger;
    using Blam::MapModel::Definitions::Type::Weap::Weap;
    using Blam::MapModel::Definitions::Type::Weap::k_TriggerActionCharge;
    using Blam::Runtime::Game::Type::Alive::AliveGame;
    using Blam::Runtime::Object::Type::Alive::AliveObject;
    using Blam::Runtime::Object::Type::Biped::Biped;
    using Blam::Runtime::Object::Type::Weapon::Weapon;
    using Perception::Details::Type::WeaponDetail;
    using Perception::Details::Type::WeaponDetails;
    using Tests::Support::Pipeline::Chain;

    constexpr std::uint32_t k_Handle{ 0x1001 };
    constexpr std::uint32_t k_Target{ 0x2001 };

    const std::string k_MagazineTag{ "objects\\weapons\\rifle\\battle_rifle\\battle_rifle" };
    const std::string k_EnergyTag{ "objects\\weapons\\rifle\\plasma_rifle\\plasma_rifle" };
    const std::string k_EmptyMagazineTag{ "objects\\weapons\\rifle\\plasma_repeater\\plasma_repeater" };
    const std::string k_ChargeTag{ "objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol" };
    const std::string k_UnknownTag{ "objects\\weapons\\unknown\\unknown" };

    class WeaponDetailsTests : public ::testing::Test
    {
    protected:
        auto SetUp() -> void override
        {
            auto& weaps{ m_Chain.m_Resolved.m_DefinitionsStore.m_Weap };
            weaps.Add(k_MagazineTag, Weap{ .Magazines = { Magazine{ .RoundsLoadedMaximum = 15 } } });
            weaps.Add(k_EnergyTag, Weap{});
            weaps.Add(k_EmptyMagazineTag, Weap{ .Magazines = { Magazine{} } });
            weaps.Add(k_ChargeTag, Weap{ .Triggers = { Trigger{ .PrimaryAction = k_TriggerActionCharge, .ChargingTime = 0.8f } } });
            weaps.Freeze();
        }

        // Publishes the weapon and the engine tick, runs the service and returns the detail of the weapon.
        auto Update(const std::string& tag, const Weapon& weapon, std::uint32_t tick) -> const WeaponDetail&
        {
            AliveObject object{};
            object.Handle = k_Handle;
            object.TagName = tag;
            object.ParentHandle = k_NoHandle;
            object.Specific = weapon;
            m_Chain.m_Tables.m_ObjectStore.RemoveObject(k_Handle);
            m_Chain.m_Tables.m_ObjectStore.AddObject(k_Handle, object);
            m_Chain.m_Tables.m_ObjectStore.Publish();
            m_Chain.m_Tables.m_GameStore.Publish(AliveGame{ .EngineTick = tick });

            m_Chain.m_Environment.m_WeaponDetailsService.Update();
            m_Published = m_Chain.m_Environment.m_WeaponDetailsStore.Acquire();
            return m_Published->at(k_Handle);
        }

        static auto Rifle(std::uint16_t loaded) -> Weapon
        {
            Weapon weapon{};
            weapon.Magazines[0] = { .Loaded = loaded, .Reserve = 60 };
            return weapon;
        }

        static auto Charging(ChargeState state) -> Weapon
        {
            Weapon weapon{};
            weapon.TotalEnergy = 1.0f;
            weapon.ChargeState = state;
            return weapon;
        }

        Chain m_Chain{};
        std::shared_ptr<const WeaponDetails> m_Published{};
    };
}

TEST_F(WeaponDetailsTests, MagazineWeaponKnowsItsAmmoAndHasNoEnergy)
{
    const WeaponDetail& detail{ this->Update(k_MagazineTag, Rifle(15), 100) };

    EXPECT_EQ(detail.Handle, k_Handle);
    EXPECT_EQ(detail.MagazineCount, 1);
    EXPECT_EQ(detail.Magazines[0].Loaded, 15);
    EXPECT_EQ(detail.Magazines[0].Reserve, 60);
    EXPECT_FALSE(detail.IsEnergyWeapon);
    EXPECT_TRUE(std::isnan(detail.Energy));
    EXPECT_TRUE(detail.CanFireNow);
    EXPECT_FALSE(detail.IsReloading);
}

TEST_F(WeaponDetailsTests, AReloadNeedsTheMagazineStateAndTheBusyTimer)
{
    Weapon reloading{ Rifle(10) };
    reloading.Magazine0ReloadState = 1;
    reloading.BusyTimer = 70;
    const WeaponDetail& during{ this->Update(k_MagazineTag, reloading, 100) };
    EXPECT_TRUE(during.IsReloading);
    EXPECT_FALSE(during.CanFireNow);

    // The reload state of the datum lasts about 14 ticks after the weapon can fire again.
    Weapon tail{ Rifle(15) };
    tail.Magazine0ReloadState = 1;
    const WeaponDetail& after{ this->Update(k_MagazineTag, tail, 101) };
    EXPECT_FALSE(after.IsReloading);
    EXPECT_TRUE(after.CanFireNow);

    // Drawing the weapon: busy, but not a reload.
    Weapon drawing{ Rifle(15) };
    drawing.BusyTimer = 38;
    const WeaponDetail& drawn{ this->Update(k_MagazineTag, drawing, 102) };
    EXPECT_FALSE(drawn.IsReloading);
    EXPECT_FALSE(drawn.CanFireNow);
}

TEST_F(WeaponDetailsTests, AnEmptyMagazineCannotFire)
{
    EXPECT_FALSE(this->Update(k_MagazineTag, Rifle(0), 100).CanFireNow);
}

TEST_F(WeaponDetailsTests, EnergyWeaponReportsItsEnergyAndStaysOverheatedWithTheHeatBelowTheThreshold)
{
    Weapon weapon{};
    weapon.TotalEnergy = 0.75f;
    weapon.TotalHeat = 0.05f;
    const WeaponDetail& ready{ this->Update(k_EnergyTag, weapon, 100) };
    EXPECT_TRUE(ready.IsEnergyWeapon);
    EXPECT_EQ(ready.MagazineCount, 0);
    EXPECT_FLOAT_EQ(ready.Energy, 0.75f);
    EXPECT_FLOAT_EQ(ready.Heat, 0.05f);
    EXPECT_FALSE(ready.IsOverheated);
    EXPECT_TRUE(ready.CanFireNow);

    // The engine says it is overheated (venting or cooling), whatever the heat says.
    weapon.IsOverheated = 1;
    const WeaponDetail& overheated{ this->Update(k_EnergyTag, weapon, 101) };
    EXPECT_TRUE(overheated.IsOverheated);
    EXPECT_FALSE(overheated.CanFireNow);

    weapon.IsOverheated = 0;
    weapon.TotalEnergy = 0.0f;
    EXPECT_FALSE(this->Update(k_EnergyTag, weapon, 102).CanFireNow);
}

TEST_F(WeaponDetailsTests, ATagMagazineOfZeroRoundsIsAnEnergyWeapon)
{
    // The plasma rifle and repeater tags carry one magazine of 0 rounds and the datum magazine stays 0/0.
    Weapon weapon{};
    weapon.TotalEnergy = 0.9f;
    const WeaponDetail& detail{ this->Update(k_EmptyMagazineTag, weapon, 100) };

    EXPECT_EQ(detail.MagazineCount, 0);
    EXPECT_TRUE(detail.IsEnergyWeapon);
    EXPECT_FLOAT_EQ(detail.Energy, 0.9f);
    EXPECT_TRUE(detail.CanFireNow);
}

TEST_F(WeaponDetailsTests, ChargeFractionCountsFromTheFirstTickInChargingOverTheChargingTime)
{
    // 0.8 s at 60 ticks is 48 ticks.
    EXPECT_FLOAT_EQ(this->Update(k_ChargeTag, Charging(ChargeState::None), 5800).ChargeFraction, 0.0f);

    const WeaponDetail& first{ this->Update(k_ChargeTag, Charging(ChargeState::Charging), 5874) };
    EXPECT_EQ(first.ChargeState, ChargeState::Charging);
    EXPECT_FLOAT_EQ(first.ChargeFraction, 0.0f);

    EXPECT_NEAR(this->Update(k_ChargeTag, Charging(ChargeState::Charging), 5880).ChargeFraction, 6.0f / 48.0f, 1e-5f);
    EXPECT_NEAR(this->Update(k_ChargeTag, Charging(ChargeState::Charging), 5898).ChargeFraction, 0.5f, 1e-5f);

    // Released before it was charged: back to zero, and the next charge starts again.
    EXPECT_FLOAT_EQ(this->Update(k_ChargeTag, Charging(ChargeState::None), 5950).ChargeFraction, 0.0f);
    EXPECT_FLOAT_EQ(this->Update(k_ChargeTag, Charging(ChargeState::Charging), 6207).ChargeFraction, 0.0f);
    EXPECT_NEAR(this->Update(k_ChargeTag, Charging(ChargeState::Charging), 6231).ChargeFraction, 0.5f, 1e-5f);
    EXPECT_FLOAT_EQ(this->Update(k_ChargeTag, Charging(ChargeState::Charged), 6255).ChargeFraction, 1.0f);
    EXPECT_FLOAT_EQ(this->Update(k_ChargeTag, Charging(ChargeState::Charging), 9999).ChargeFraction, 0.0f);
    EXPECT_FLOAT_EQ(this->Update(k_ChargeTag, Charging(ChargeState::Charging), 10100).ChargeFraction, 1.0f);
}

TEST_F(WeaponDetailsTests, ACleanupForgetsWhenTheChargeStarted)
{
    this->Update(k_ChargeTag, Charging(ChargeState::Charging), 100);
    EXPECT_NEAR(this->Update(k_ChargeTag, Charging(ChargeState::Charging), 110).ChargeFraction, 10.0f / 48.0f, 1e-5f);

    m_Chain.m_Environment.m_WeaponDetailsService.Cleanup();

    // Seen already charging after a load: it starts from this tick.
    EXPECT_FLOAT_EQ(this->Update(k_ChargeTag, Charging(ChargeState::Charging), 120).ChargeFraction, 0.0f);
}

TEST_F(WeaponDetailsTests, WeaponWithoutAChargingTriggerHasNoChargeFraction)
{
    EXPECT_TRUE(std::isnan(this->Update(k_EnergyTag, Charging(ChargeState::None), 100).ChargeFraction));
    EXPECT_TRUE(std::isnan(this->Update(k_MagazineTag, Charging(ChargeState::Charging), 101).ChargeFraction));
}

TEST_F(WeaponDetailsTests, TrackingWeaponKnowsItsTargetAndWhetherItIsLocked)
{
    Weapon rocket{ Rifle(1) };
    rocket.IsTracking = 1;
    rocket.TrackedObject = k_Target;

    rocket.TrackingState = 1;
    const WeaponDetail& aiming{ this->Update(k_MagazineTag, rocket, 100) };
    EXPECT_EQ(aiming.TrackedObject, k_Target);
    EXPECT_FALSE(aiming.IsLocked);

    rocket.TrackingState = 2;
    EXPECT_TRUE(this->Update(k_MagazineTag, rocket, 101).IsLocked);

    // The lock is lost (state 3) but the weapon still points at the object.
    rocket.TrackingState = 3;
    const WeaponDetail& lost{ this->Update(k_MagazineTag, rocket, 102) };
    EXPECT_FALSE(lost.IsLocked);
    EXPECT_EQ(lost.TrackedObject, k_Target);

    // Not tracking: the object left in the datum is not reported.
    rocket.IsTracking = 0;
    rocket.TrackingState = 2;
    const WeaponDetail& idle{ this->Update(k_MagazineTag, rocket, 103) };
    EXPECT_EQ(idle.TrackedObject, k_NoHandle);
    EXPECT_FALSE(idle.IsLocked);
}

TEST_F(WeaponDetailsTests, WeaponWithoutADefinitionKeepsTheNeutralValuesOfTheTag)
{
    const WeaponDetail& detail{ this->Update(k_UnknownTag, Rifle(5), 100) };

    EXPECT_EQ(detail.MagazineCount, 0);
    EXPECT_FALSE(detail.IsEnergyWeapon);
    EXPECT_TRUE(std::isnan(detail.Energy));
    EXPECT_TRUE(std::isnan(detail.ChargeFraction));
    // What the datum says still counts.
    EXPECT_EQ(detail.Magazines[0].Loaded, 0);
    EXPECT_TRUE(detail.CanFireNow);
}

TEST_F(WeaponDetailsTests, OnlyWeaponsGetADetailAndCleanupDropsThem)
{
    AliveObject biped{};
    biped.Handle = k_Target;
    biped.ParentHandle = k_NoHandle;
    biped.Specific = Biped{};
    m_Chain.m_Tables.m_ObjectStore.AddObject(k_Target, biped);

    const WeaponDetail& detail{ this->Update(k_MagazineTag, Rifle(1), 100) };
    EXPECT_EQ(detail.Handle, k_Handle);
    EXPECT_EQ(m_Published->size(), 1u);

    m_Chain.m_Environment.m_WeaponDetailsService.Cleanup();
    EXPECT_EQ(m_Chain.m_Environment.m_WeaponDetailsStore.Acquire(), nullptr);
}