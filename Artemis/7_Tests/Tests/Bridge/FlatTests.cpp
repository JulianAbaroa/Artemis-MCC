#include <gtest/gtest.h>

import Common.Engine.Type;
import Common.Math.Type;
import Common.Semantic.Type;
import Common.Team.Type;
import Contract.Bridge.Type;
import Blam.Layout.Player.Type;
import Bridge.Codec.Flat.System;
import Bridge.Codec.Tick.Type;
import Perception.Details.Type;
import Perception.Events.Type;
import Perception.Match.Type;
import Perception.Players.Type;
import Views.Raycast.Type;
import Views.View.Type;
import std;

// The flags, the stands, the round and the team scores in the BridgeTick (B4h H.4), and the two sequences of an event.

namespace
{
    using Bridge::Codec::Flat::System::FlatService;
    using Bridge::Codec::Tick::Type::Tick;
    using Common::Math::Type::Vec3;
    using Common::Semantic::Type::ChargeState;
    using Common::Semantic::Type::Event;
    using Common::Semantic::Type::Function;
    using Common::Semantic::Type::FlagState;
    using Common::Semantic::Type::RoundState;
    using Common::Team::Type::Team;
    using Blam::Layout::Player::Type::Traits::PlayerTraits;
    using Contract::Bridge::Type::BridgeGround;
    using Contract::Bridge::Type::BridgeHitKind;
    using Contract::Bridge::Type::BridgeView;
    using Contract::Bridge::Type::BridgeViewFlagSide;
    using Contract::Bridge::Type::BridgeViewMode;
    using Contract::Bridge::Type::BridgeViewRelation;
    using Contract::Bridge::Type::BridgeViewSoundKind;
    using Contract::Bridge::Type::BridgeInfiniteAmmo;
    using Contract::Bridge::Type::BridgeVehicleUse;
    using Contract::Bridge::Type::BridgeIdentityVisibility;
    using Contract::Bridge::Type::BridgeRadarMode;
    using Contract::Bridge::Type::BridgePowerup;
    using Contract::Bridge::Type::BridgeTick;
    using Contract::Bridge::Type::BridgeTickFlag;
    using Perception::Details::Type::ExplosiveDetail;
    using Perception::Details::Type::ExplosiveDetails;
    using Perception::Details::Type::LiftDetail;
    using Perception::Details::Type::LiftDetails;
    using Perception::Details::Type::LiftStage;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ObjectiveDetails;
    using Perception::Details::Type::ProjectileDetail;
    using Perception::Details::Type::ProjectileDetails;
    using Perception::Details::Type::StandDetail;
    using Perception::Details::Type::WeaponDetail;
    using Perception::Details::Type::WeaponDetails;
    using Perception::Events::Type::EventLog;
    using Perception::Events::Type::GameEvent;
    using Perception::Match::Type::MatchStatus;
    using Perception::Players::Type::Body;
    using Perception::Players::Type::GroundKind;
    using Perception::Players::Type::Player;
    using Perception::Players::Type::Players;
    using Views::Raycast::Type::HitKind;
    using Views::View::Type::View;
    using Views::View::Type::ViewFlagSide;
    using Views::View::Type::ViewMode;
    using Views::View::Type::ViewRelation;
    using Views::View::Type::ViewSoundKind;
    using Views::View::Type::ViewTable;

    using Contract::Bridge::Type::k_BridgeMaxExplosives;
    using Contract::Bridge::Type::k_BridgeMaxLifts;
    using Contract::Bridge::Type::k_BridgeMaxLiftStages;
    using Contract::Bridge::Type::k_BridgeMaxObjectives;
    using Contract::Bridge::Type::k_BridgeMaxProjectiles;
    using Contract::Bridge::Type::k_BridgeMaxWeapons;
    using Common::Engine::Type::k_NoHandle;

    auto Flat(const Tick& tick) -> std::unique_ptr<BridgeTick>
    {
        auto flat{ std::make_unique<BridgeTick>() };
        FlatService{}.Flatten(tick, *flat);
        return flat;
    }

    // Every field of a View gets the next number after the base, in the order of the type. ExpectView walks the same order.
    auto FillView(View& view, std::uint32_t base) -> void
    {
        std::uint32_t n{ base };
        const auto next{ [&n] { return ++n; } };
        const auto vec{ [&next] { return Vec3{ .X = static_cast<float>(next()), .Y = static_cast<float>(next()), .Z = static_cast<float>(next()) }; } };
        const auto relation{ [&next] { return static_cast<ViewRelation>(1 + next() % 4); } };

        view.Head = { .PlayerHandle = next(), .PlayerRow = static_cast<std::uint8_t>(next()), .LocalIndex = static_cast<std::uint8_t>(next()), .BipedHandle = next(),
            .Mode = ViewMode::HumanLike, .IsComputed = true, .HasBody = true, .Team = Team::Red, .Side = Team::Blue };
        view.Self.Eye = vec();
        view.Self.Feet = vec();
        view.Self.FrameYaw = static_cast<float>(next());
        view.Self.AimPitch = static_cast<float>(next());
        view.Self.LinearVelocity = vec();
        view.Self.OwnVelocity = vec();
        view.Self.Vitality = std::numeric_limits<float>::quiet_NaN();
        view.Self.Shield = static_cast<float>(next());
        view.Self.Ground = GroundKind::Object;
        view.Self.IsGrounded = true;
        view.Self.AirborneTicks = next();
        view.Self.CrouchFraction = static_cast<float>(next());
        view.Self.TicksAlive = next();
        view.Self.RespawnTicks = next();
        view.Self.PowerupTicks = next();
        view.Self.FragGrenades = static_cast<std::uint8_t>(next());
        view.Self.PlasmaGrenades = static_cast<std::uint8_t>(next());
        view.Self.Ability = next();
        view.Self.AbilityEnergy = static_cast<float>(next());
        view.Self.IsAbilityActive = true;
        view.Self.CarriesFlag = true;
        view.Self.TicksSinceDamage = next();
        view.Self.IsTracked = true;
        for (auto& weapon : view.Self.Weapons)
        {
            weapon = { .Handle = next(), .Subtype = static_cast<std::uint8_t>(next()), .IsEquipped = true, .Loaded = static_cast<std::uint16_t>(next()),
                .Reserve = static_cast<std::uint16_t>(next()), .Energy = static_cast<float>(next()), .Heat = static_cast<float>(next()),
                .IsOverheated = true, .IsReloading = true, .ChargeFraction = static_cast<float>(next()), .CanFireNow = true };
        }
        view.AimHit = { .Distance = static_cast<float>(next()), .MaxDistance = static_cast<float>(next()), .Point = vec(), .Material = static_cast<std::int16_t>(next()),
            .Kind = HitKind::Dynamic, .ObjectHandle = next(), .Function = static_cast<Function>(next()), .Subtype = static_cast<std::uint8_t>(next()),
            .Relation = relation(), .PlayerRow = static_cast<std::uint8_t>(next()), .RegionIndex = static_cast<std::int16_t>(next()), .IsHead = true,
            .SurfaceFlags = static_cast<std::uint8_t>(next()) };
        view.RayOrigin = vec();
        for (auto& ray : view.Rays)
        {
            ray = { .Distance = static_cast<float>(next()), .Kind = HitKind::Static, .ObjectHandle = next(), .Material = static_cast<std::int16_t>(next()),
                .Function = static_cast<Function>(next()), .Relation = relation(), .SurfaceFlags = static_cast<std::uint8_t>(next()) };
        }
        for (auto& other : view.Others)
        {
            other = { .Relation = relation(), .Channels = static_cast<std::uint8_t>(next() & 7), .IsAlive = true, .Position = vec(), .Velocity = vec(),
                .FacingYaw = static_cast<float>(next()), .Vitality = std::numeric_limits<float>::quiet_NaN(), .Shield = static_cast<float>(next()),
                .WeaponSubtype = static_cast<std::uint8_t>(next()), .IsCrouching = true, .CarriesFlag = true, .Elevation = -1 };
        }
        for (auto& flag : view.Flags)
        {
            flag = { .State = FlagState::Dropped, .Position = vec(), .CarrierRow = static_cast<std::uint8_t>(next()), .HasFlag = (next() % 2) != 0, .ResetTicks = next(), .IsResetRunning = true,
                .AlliesInZone = static_cast<std::uint8_t>(next()), .StandPosition = vec() };
        }
        view.SoundCount = static_cast<std::uint8_t>(next());
        for (auto& sound : view.Sounds)
        {
            sound = { .Kind = ViewSoundKind::Explosion, .SourceFunction = static_cast<Function>(next()), .SourceSubtype = static_cast<std::uint8_t>(next()),
                .Direction = vec(), .Intensity = static_cast<float>(next()), .AgeTicks = next() };
        }
        view.NoticeCount = static_cast<std::uint8_t>(next());
        for (auto& notice : view.Notices)
        {
            notice = { .Kind = Event::FlagTaken, .CauseRow = static_cast<std::uint8_t>(next()), .EffectRow = static_cast<std::uint8_t>(next()),
                .CauseRelation = relation(), .EffectRelation = relation(), .FlagSide = ViewFlagSide::Rival, .AgeTicks = next() };
        }
    }

    auto ExpectVec(const Vec3& expected, const Contract::Bridge::Type::BridgeVec3& row) -> void
    {
        EXPECT_EQ(row.X, expected.X);
        EXPECT_EQ(row.Y, expected.Y);
        EXPECT_EQ(row.Z, expected.Z);
    }

    // Compares the bridge row with the View it was copied from, field by field. A float that is NaN in the View must be NaN in the row.
    auto ExpectView(const View& view, const BridgeView& row) -> void
    {
        EXPECT_EQ(row.Head.PlayerHandle, view.Head.PlayerHandle);
        EXPECT_EQ(row.Head.PlayerRow, view.Head.PlayerRow);
        EXPECT_EQ(row.Head.LocalIndex, view.Head.LocalIndex);
        EXPECT_EQ(row.Head.BipedHandle, view.Head.BipedHandle);
        EXPECT_EQ(row.Head.Mode, BridgeViewMode::HumanLike);
        EXPECT_EQ(row.Head.IsComputed, 1);
        EXPECT_EQ(row.Head.HasBody, 1);
        EXPECT_EQ(row.Head.Team, static_cast<std::uint8_t>(Team::Red));
        EXPECT_EQ(row.Head.Side, static_cast<std::uint8_t>(Team::Blue));

        const auto& self{ view.Self };
        ExpectVec(self.Eye, row.Self.Eye);
        ExpectVec(self.Feet, row.Self.Feet);
        EXPECT_EQ(row.Self.FrameYaw, self.FrameYaw);
        EXPECT_EQ(row.Self.AimPitch, self.AimPitch);
        ExpectVec(self.LinearVelocity, row.Self.LinearVelocity);
        ExpectVec(self.OwnVelocity, row.Self.OwnVelocity);
        EXPECT_TRUE(std::isnan(row.Self.Vitality));
        EXPECT_EQ(row.Self.Shield, self.Shield);
        EXPECT_EQ(row.Self.Ground, BridgeGround::Object);
        EXPECT_EQ(row.Self.IsGrounded, 1);
        EXPECT_EQ(row.Self.AirborneTicks, self.AirborneTicks);
        EXPECT_EQ(row.Self.CrouchFraction, self.CrouchFraction);
        EXPECT_EQ(row.Self.Zoom, static_cast<std::uint8_t>(self.Zoom));
        EXPECT_EQ(row.Self.TicksAlive, self.TicksAlive);
        EXPECT_EQ(row.Self.RespawnTicks, self.RespawnTicks);
        EXPECT_EQ(row.Self.Powerup, BridgePowerup::NoPowerup);
        EXPECT_EQ(row.Self.PowerupTicks, self.PowerupTicks);
        EXPECT_EQ(row.Self.SelectedGrenade, static_cast<std::uint8_t>(self.SelectedGrenade));
        EXPECT_EQ(row.Self.FragGrenades, self.FragGrenades);
        EXPECT_EQ(row.Self.PlasmaGrenades, self.PlasmaGrenades);
        EXPECT_EQ(row.Self.Ability, self.Ability);
        EXPECT_EQ(row.Self.AbilityEnergy, self.AbilityEnergy);
        EXPECT_EQ(row.Self.IsAbilityActive, 1);
        EXPECT_EQ(row.Self.CarriesFlag, 1);
        EXPECT_EQ(row.Self.TicksSinceDamage, self.TicksSinceDamage);
        EXPECT_EQ(row.Self.IsTracked, 1);
        for (std::size_t i{ 0 }; i < self.Weapons.size(); ++i)
        {
            const auto& weapon{ self.Weapons[i] };
            const auto& out{ row.Self.Weapons[i] };
            EXPECT_EQ(out.Handle, weapon.Handle);
            EXPECT_EQ(out.Subtype, weapon.Subtype);
            EXPECT_EQ(out.IsEquipped, 1);
            EXPECT_EQ(out.Loaded, weapon.Loaded);
            EXPECT_EQ(out.Reserve, weapon.Reserve);
            EXPECT_EQ(out.Energy, weapon.Energy);
            EXPECT_EQ(out.Heat, weapon.Heat);
            EXPECT_EQ(out.IsOverheated, 1);
            EXPECT_EQ(out.IsReloading, 1);
            EXPECT_EQ(out.ChargeFraction, weapon.ChargeFraction);
            EXPECT_EQ(out.CanFireNow, 1);
        }

        const auto& aim{ view.AimHit };
        EXPECT_EQ(row.AimHit.Distance, aim.Distance);
        EXPECT_EQ(row.AimHit.MaxDistance, aim.MaxDistance);
        ExpectVec(aim.Point, row.AimHit.Point);
        EXPECT_EQ(row.AimHit.Material, aim.Material);
        EXPECT_EQ(row.AimHit.Kind, BridgeHitKind::Dynamic);
        EXPECT_EQ(row.AimHit.ObjectHandle, aim.ObjectHandle);
        EXPECT_EQ(row.AimHit.Function, static_cast<std::uint8_t>(aim.Function));
        EXPECT_EQ(row.AimHit.Subtype, aim.Subtype);
        EXPECT_EQ(static_cast<std::uint8_t>(row.AimHit.Relation), static_cast<std::uint8_t>(aim.Relation));
        EXPECT_EQ(row.AimHit.PlayerRow, aim.PlayerRow);
        EXPECT_EQ(row.AimHit.RegionIndex, aim.RegionIndex);
        EXPECT_EQ(row.AimHit.IsHead, 1);
        EXPECT_EQ(row.AimHit.SurfaceFlags, aim.SurfaceFlags);

        ExpectVec(view.RayOrigin, row.RayOrigin);
        for (std::size_t i{ 0 }; i < view.Rays.size(); ++i)
        {
            const auto& ray{ view.Rays[i] };
            EXPECT_EQ(row.Rays[i].Distance, ray.Distance);
            EXPECT_EQ(row.Rays[i].Kind, BridgeHitKind::Static);
            EXPECT_EQ(row.Rays[i].ObjectHandle, ray.ObjectHandle);
            EXPECT_EQ(row.Rays[i].Material, ray.Material);
            EXPECT_EQ(row.Rays[i].Function, static_cast<std::uint8_t>(ray.Function));
            EXPECT_EQ(static_cast<std::uint8_t>(row.Rays[i].Relation), static_cast<std::uint8_t>(ray.Relation));
            EXPECT_EQ(row.Rays[i].SurfaceFlags, ray.SurfaceFlags);
        }
        for (std::size_t i{ 0 }; i < view.Others.size(); ++i)
        {
            const auto& other{ view.Others[i] };
            EXPECT_EQ(static_cast<std::uint8_t>(row.Others[i].Relation), static_cast<std::uint8_t>(other.Relation));
            EXPECT_EQ(row.Others[i].Channels, other.Channels);
            EXPECT_EQ(row.Others[i].IsAlive, 1);
            ExpectVec(other.Position, row.Others[i].Position);
            ExpectVec(other.Velocity, row.Others[i].Velocity);
            EXPECT_EQ(row.Others[i].FacingYaw, other.FacingYaw);
            EXPECT_TRUE(std::isnan(row.Others[i].Vitality));
            EXPECT_EQ(row.Others[i].Shield, other.Shield);
            EXPECT_EQ(row.Others[i].WeaponSubtype, other.WeaponSubtype);
            EXPECT_EQ(row.Others[i].IsCrouching, 1);
            EXPECT_EQ(row.Others[i].CarriesFlag, 1);
            EXPECT_EQ(row.Others[i].Elevation, -1);
        }
        for (std::size_t i{ 0 }; i < view.Flags.size(); ++i)
        {
            const auto& flag{ view.Flags[i] };
            EXPECT_EQ(row.Flags[i].State, static_cast<std::uint8_t>(FlagState::Dropped));
            ExpectVec(flag.Position, row.Flags[i].Position);
            EXPECT_EQ(row.Flags[i].CarrierRow, flag.CarrierRow);
            EXPECT_EQ(row.Flags[i].HasFlag, flag.HasFlag ? 1 : 0);
            EXPECT_EQ(row.Flags[i].ResetTicks, flag.ResetTicks);
            EXPECT_EQ(row.Flags[i].IsResetRunning, 1);
            EXPECT_EQ(row.Flags[i].AlliesInZone, flag.AlliesInZone);
            ExpectVec(flag.StandPosition, row.Flags[i].StandPosition);
        }
        EXPECT_EQ(row.SoundCount, view.SoundCount);
        for (std::size_t i{ 0 }; i < view.Sounds.size(); ++i)
        {
            const auto& sound{ view.Sounds[i] };
            EXPECT_EQ(row.Sounds[i].Kind, BridgeViewSoundKind::Explosion);
            EXPECT_EQ(row.Sounds[i].SourceFunction, static_cast<std::uint8_t>(sound.SourceFunction));
            EXPECT_EQ(row.Sounds[i].SourceSubtype, sound.SourceSubtype);
            ExpectVec(sound.Direction, row.Sounds[i].Direction);
            EXPECT_EQ(row.Sounds[i].Intensity, sound.Intensity);
            EXPECT_EQ(row.Sounds[i].AgeTicks, sound.AgeTicks);
        }
        EXPECT_EQ(row.NoticeCount, view.NoticeCount);
        for (std::size_t i{ 0 }; i < view.Notices.size(); ++i)
        {
            const auto& notice{ view.Notices[i] };
            EXPECT_EQ(row.Notices[i].Kind, static_cast<std::uint8_t>(Event::FlagTaken));
            EXPECT_EQ(row.Notices[i].CauseRow, notice.CauseRow);
            EXPECT_EQ(row.Notices[i].EffectRow, notice.EffectRow);
            EXPECT_EQ(static_cast<std::uint8_t>(row.Notices[i].CauseRelation), static_cast<std::uint8_t>(notice.CauseRelation));
            EXPECT_EQ(static_cast<std::uint8_t>(row.Notices[i].EffectRelation), static_cast<std::uint8_t>(notice.EffectRelation));
            EXPECT_EQ(row.Notices[i].FlagSide, BridgeViewFlagSide::Rival);
            EXPECT_EQ(row.Notices[i].AgeTicks, notice.AgeTicks);
        }
    }

    // return: A Tick with this many players and, when views is not null, those Views.
    auto WithViews(std::size_t playerCount, std::shared_ptr<const ViewTable> views) -> Tick
    {
        auto players{ std::make_shared<Players>() };
        for (std::size_t i{ 0 }; i < playerCount; ++i) players->Rows.push_back(Player{ .Handle = static_cast<std::uint32_t>(0xE5A10000 + i) });
        Tick tick{};
        tick.Players = players;
        tick.Views = std::move(views);
        return tick;
    }

    auto Flag(std::uint32_t handle, Team baseTeam) -> ObjectiveDetail
    {
        return ObjectiveDetail{ .Handle = handle, .BaseTeam = baseTeam, .OwnerTeam = baseTeam };
    }

    auto WithObjectives(std::shared_ptr<const ObjectiveDetails> objectives) -> Tick
    {
        Tick tick{};
        tick.ObjectiveDetails = std::move(objectives);
        return tick;
    }
}

TEST(FlatTests, FlagFieldsAreCopiedToTheObjectiveRow)
{
    ObjectiveDetails objectives{};
    objectives.Flags[0xE5A10010] = ObjectiveDetail{ .Handle = 0xE5A10010, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red,
        .State = FlagState::Dropped, .CarrierPlayer = 0xE5A20003, .CarrierBiped = 0xE5A30004, .HomeStand = 0xE5A40005, .ResetTicks = 321,
        .IsResetRunning = true, .AlliesInZone = 2 };

    const auto flat{ Flat(WithObjectives(std::make_shared<const ObjectiveDetails>(std::move(objectives)))) };

    ASSERT_EQ(flat->Header.ObjectiveCount, 1u);
    EXPECT_EQ(flat->Header.StandCount, 0u);
    const auto& row{ flat->Objectives[0] };
    EXPECT_EQ(row.Handle, 0xE5A10010u);
    EXPECT_EQ(row.BaseTeam, static_cast<std::uint8_t>(Team::Blue));
    EXPECT_EQ(row.OwnerTeam, static_cast<std::uint8_t>(Team::Red));
    EXPECT_EQ(row.State, static_cast<std::uint8_t>(FlagState::Dropped));
    EXPECT_EQ(row.CarrierPlayer, 0xE5A20003u);
    EXPECT_EQ(row.CarrierBiped, 0xE5A30004u);
    EXPECT_EQ(row.HomeStand, 0xE5A40005u);
    EXPECT_EQ(row.ResetTicks, 321u);
    EXPECT_EQ(row.IsResetRunning, 1u);
    EXPECT_EQ(row.AlliesInZone, 2u);
    EXPECT_EQ(flat->Header.Flags, 0u);
}

TEST(FlatTests, FlagsAndStandsAreOrderedByBaseTeamThenHandle)
{
    // An unordered map gives no order: the rows must not depend on it.
    ObjectiveDetails objectives{};
    objectives.Flags[0x30] = Flag(0x30, Team::Blue);
    objectives.Flags[0x20] = Flag(0x20, Team::Red);
    objectives.Flags[0x10] = Flag(0x10, Team::Blue);
    objectives.Stands[0x99] = StandDetail{ .Handle = 0x99, .BaseTeam = Team::Blue, .OwnerTeam = Team::Red };
    objectives.Stands[0x88] = StandDetail{ .Handle = 0x88, .BaseTeam = Team::Red, .OwnerTeam = Team::Blue };

    const auto flat{ Flat(WithObjectives(std::make_shared<const ObjectiveDetails>(std::move(objectives)))) };

    ASSERT_EQ(flat->Header.ObjectiveCount, 3u);
    EXPECT_EQ(flat->Objectives[0].Handle, 0x20u);
    EXPECT_EQ(flat->Objectives[1].Handle, 0x10u);
    EXPECT_EQ(flat->Objectives[2].Handle, 0x30u);

    ASSERT_EQ(flat->Header.StandCount, 2u);
    EXPECT_EQ(flat->Stands[0].Handle, 0x88u);
    EXPECT_EQ(flat->Stands[0].BaseTeam, static_cast<std::uint8_t>(Team::Red));
    EXPECT_EQ(flat->Stands[0].OwnerTeam, static_cast<std::uint8_t>(Team::Blue));
    EXPECT_EQ(flat->Stands[1].Handle, 0x99u);
}

TEST(FlatTests, MoreFlagsThanTheCapacityAreCutAndFlagged)
{
    ObjectiveDetails objectives{};
    for (std::uint32_t i{ 0 }; i < k_BridgeMaxObjectives + 1; ++i) objectives.Flags[0x100 + i] = Flag(0x100 + i, Team::Red);

    const auto flat{ Flat(WithObjectives(std::make_shared<const ObjectiveDetails>(std::move(objectives)))) };

    EXPECT_EQ(flat->Header.ObjectiveCount, k_BridgeMaxObjectives);
    EXPECT_NE(flat->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::ObjectivesTruncated), 0u);
}

TEST(FlatTests, WithoutObjectiveDetailsTheCountsAreZero)
{
    const auto flat{ Flat(Tick{}) };

    EXPECT_EQ(flat->Header.ObjectiveCount, 0u);
    EXPECT_EQ(flat->Header.StandCount, 0u);
    EXPECT_EQ(flat->Header.Flags, 0u);
}

TEST(FlatTests, WeaponFieldsAreCopiedToTheWeaponRow)
{
    WeaponDetails weapons{};
    WeaponDetail detail{ .Handle = 0xE4370001, .MagazineCount = 1, .IsEnergyWeapon = false, .Energy = std::numeric_limits<float>::quiet_NaN(),
        .Heat = 0.25f, .IsOverheated = true, .IsReloading = true, .ChargeState = ChargeState::Charging, .ChargeFraction = 0.5f,
        .TrackedObject = 0xE4280002, .IsLocked = true, .CanFireNow = false };
    detail.Magazines[0] = { .Loaded = 12, .Reserve = 48 };
    detail.Magazines[1] = { .Loaded = 3, .Reserve = 9 };
    weapons[detail.Handle] = detail;

    Tick tick{};
    tick.WeaponDetails = std::make_shared<const WeaponDetails>(std::move(weapons));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.WeaponCount, 1u);
    EXPECT_EQ(flat->Header.ProjectileCount, 0u);
    const auto& row{ flat->Weapons[0] };
    EXPECT_EQ(row.Handle, 0xE4370001u);
    EXPECT_EQ(row.MagazineCount, 1u);
    EXPECT_EQ(row.Magazines[0].Loaded, 12u);
    EXPECT_EQ(row.Magazines[0].Reserve, 48u);
    EXPECT_EQ(row.Magazines[1].Loaded, 3u);
    EXPECT_EQ(row.Magazines[1].Reserve, 9u);
    EXPECT_EQ(row.IsEnergyWeapon, 0u);
    EXPECT_TRUE(std::isnan(row.Energy));
    EXPECT_FLOAT_EQ(row.Heat, 0.25f);
    EXPECT_EQ(row.IsOverheated, 1u);
    EXPECT_EQ(row.IsReloading, 1u);
    EXPECT_EQ(row.ChargeState, static_cast<std::uint8_t>(ChargeState::Charging));
    EXPECT_FLOAT_EQ(row.ChargeFraction, 0.5f);
    EXPECT_EQ(row.TrackedObject, 0xE4280002u);
    EXPECT_EQ(row.IsLocked, 1u);
    EXPECT_EQ(row.CanFireNow, 0u);
    EXPECT_EQ(flat->Header.Flags, 0u);
}

TEST(FlatTests, ProjectileFieldsAreCopiedToTheProjectileRow)
{
    ProjectileDetails projectiles{};
    projectiles[0xE4E30001] = ProjectileDetail{ .Handle = 0xE4E30001, .OwnerPlayer = 0xEC700000, .OwnerBiped = 0xE43301C4,
        .ShooterRoot = 0xE4440005, .StuckTo = 0xE4D50006, .StuckNode = 7, .InHand = false };
    projectiles[0xE4E30002] = ProjectileDetail{ .Handle = 0xE4E30002, .OwnerPlayer = 0xEC700000, .OwnerBiped = 0xE43301C4,
        .ShooterRoot = 0xE43301C4, .StuckTo = k_NoHandle, .StuckNode = -1, .InHand = true };

    Tick tick{};
    tick.ProjectileDetails = std::make_shared<const ProjectileDetails>(std::move(projectiles));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.ProjectileCount, 2u);
    EXPECT_EQ(flat->Header.WeaponCount, 0u);
    const auto& stuck{ flat->Projectiles[0] };
    EXPECT_EQ(stuck.Handle, 0xE4E30001u);
    EXPECT_EQ(stuck.OwnerPlayer, 0xEC700000u);
    EXPECT_EQ(stuck.OwnerBiped, 0xE43301C4u);
    EXPECT_EQ(stuck.ShooterRoot, 0xE4440005u);
    EXPECT_EQ(stuck.StuckTo, 0xE4D50006u);
    EXPECT_EQ(stuck.StuckNode, 7);
    EXPECT_EQ(stuck.InHand, 0u);
    const auto& inHand{ flat->Projectiles[1] };
    EXPECT_EQ(inHand.StuckTo, k_NoHandle);
    EXPECT_EQ(inHand.StuckNode, -1);
    EXPECT_EQ(inHand.InHand, 1u);
}

TEST(FlatTests, WeaponsAndProjectilesAreOrderedByHandle)
{
    // An unordered map gives no order: the rows must not depend on it.
    WeaponDetails weapons{};
    for (const std::uint32_t handle : { 0x30u, 0x10u, 0x20u }) weapons[handle] = WeaponDetail{ .Handle = handle };
    ProjectileDetails projectiles{};
    for (const std::uint32_t handle : { 0x90u, 0x70u, 0x80u }) projectiles[handle] = ProjectileDetail{ .Handle = handle };

    Tick tick{};
    tick.WeaponDetails = std::make_shared<const WeaponDetails>(std::move(weapons));
    tick.ProjectileDetails = std::make_shared<const ProjectileDetails>(std::move(projectiles));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.WeaponCount, 3u);
    EXPECT_EQ(flat->Weapons[0].Handle, 0x10u);
    EXPECT_EQ(flat->Weapons[1].Handle, 0x20u);
    EXPECT_EQ(flat->Weapons[2].Handle, 0x30u);
    ASSERT_EQ(flat->Header.ProjectileCount, 3u);
    EXPECT_EQ(flat->Projectiles[0].Handle, 0x70u);
    EXPECT_EQ(flat->Projectiles[1].Handle, 0x80u);
    EXPECT_EQ(flat->Projectiles[2].Handle, 0x90u);
}

TEST(FlatTests, MoreWeaponsOrProjectilesThanTheCapacityAreCutAndFlagged)
{
    WeaponDetails weapons{};
    for (std::uint32_t i{ 0 }; i < k_BridgeMaxWeapons + 1; ++i) weapons[0x100 + i] = WeaponDetail{ .Handle = 0x100 + i };

    Tick onlyWeapons{};
    onlyWeapons.WeaponDetails = std::make_shared<const WeaponDetails>(std::move(weapons));
    const auto flatWeapons{ Flat(onlyWeapons) };
    EXPECT_EQ(flatWeapons->Header.WeaponCount, k_BridgeMaxWeapons);
    EXPECT_NE(flatWeapons->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::WeaponsTruncated), 0u);
    EXPECT_EQ(flatWeapons->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::ProjectilesTruncated), 0u);
    // The cut keeps the lowest handles.
    EXPECT_EQ(flatWeapons->Weapons[k_BridgeMaxWeapons - 1].Handle, 0x100u + k_BridgeMaxWeapons - 1);

    ProjectileDetails projectiles{};
    for (std::uint32_t i{ 0 }; i < k_BridgeMaxProjectiles + 1; ++i) projectiles[0x100 + i] = ProjectileDetail{ .Handle = 0x100 + i };

    Tick onlyProjectiles{};
    onlyProjectiles.ProjectileDetails = std::make_shared<const ProjectileDetails>(std::move(projectiles));
    const auto flatProjectiles{ Flat(onlyProjectiles) };
    EXPECT_EQ(flatProjectiles->Header.ProjectileCount, k_BridgeMaxProjectiles);
    EXPECT_NE(flatProjectiles->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::ProjectilesTruncated), 0u);
    EXPECT_EQ(flatProjectiles->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::WeaponsTruncated), 0u);
}

TEST(FlatTests, ExplosivesAreOrderedByHandleCarryTheirBlastAndAreCutAtTheCapacity)
{
    ExplosiveDetails explosives{};
    explosives[0x20] = ExplosiveDetail{ .Handle = 0x20, .RadiusMin = 0.8f, .RadiusMax = 2.0f, .Damage = 200.0f };
    explosives[0x10] = ExplosiveDetail{ .Handle = 0x10, .RadiusMin = 0.6f, .RadiusMax = 2.0f, .Damage = 600.0f };

    Tick tick{};
    tick.ExplosiveDetails = std::make_shared<const ExplosiveDetails>(std::move(explosives));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.ExplosiveCount, 2u);
    EXPECT_EQ(flat->Explosives[0].Handle, 0x10u);
    EXPECT_FLOAT_EQ(flat->Explosives[0].Damage, 600.0f);
    EXPECT_EQ(flat->Explosives[1].Handle, 0x20u);
    EXPECT_FLOAT_EQ(flat->Explosives[1].RadiusMin, 0.8f);
    EXPECT_FLOAT_EQ(flat->Explosives[1].RadiusMax, 2.0f);
    EXPECT_FLOAT_EQ(flat->Explosives[1].Damage, 200.0f);
    EXPECT_EQ(flat->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::ExplosivesTruncated), 0u);

    ExplosiveDetails many{};
    for (std::uint32_t i{ 0 }; i < k_BridgeMaxExplosives + 1; ++i) many[0x100 + i] = ExplosiveDetail{ .Handle = 0x100 + i };
    Tick full{};
    full.ExplosiveDetails = std::make_shared<const ExplosiveDetails>(std::move(many));
    const auto flatFull{ Flat(full) };
    EXPECT_EQ(flatFull->Header.ExplosiveCount, k_BridgeMaxExplosives);
    EXPECT_NE(flatFull->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::ExplosivesTruncated), 0u);
}

TEST(FlatTests, LiftsAreOrderedByHandleCarryTheirStagesAndExitAndAreCutAtTheCapacity)
{
    LiftStage first{ .Position = Vec3{ 1.0f, 2.0f, 3.0f }, .Direction = Vec3{ 0.0f, -0.82f, 0.57f }, .Acceleration = 11.0f, .MaximumVelocity = 7.5f,
        .AxisAcceleration = 25.0f, .AxisMaximumVelocity = 20.0f, .Affects = 15 };
    LiftStage second{ .Direction = Vec3{ 0.0f, 0.0f, 1.0f }, .Acceleration = 16.0f, .MaximumVelocity = 8.0f, .Affects = 3 };

    LiftDetails lifts{};
    lifts[0x20] = LiftDetail{ .Handle = 0x20, .Stages = { first, second }, .HasExit = true, .ExitTick = 1227,
        .ExitVelocity = Vec3{ 0.252f, -6.162f, 4.114f }, .ExitBiped = 0xB1 };
    lifts[0x10] = LiftDetail{ .Handle = 0x10 };

    Tick tick{};
    tick.LiftDetails = std::make_shared<const LiftDetails>(std::move(lifts));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.LiftCount, 2u);
    EXPECT_EQ(flat->Lifts[0].Handle, 0x10u);
    EXPECT_EQ(flat->Lifts[0].StageCount, 0u);
    EXPECT_EQ(flat->Lifts[0].HasExit, 0u);
    EXPECT_EQ(flat->Lifts[0].ExitBiped, k_NoHandle);

    const auto& lift{ flat->Lifts[1] };
    EXPECT_EQ(lift.Handle, 0x20u);
    ASSERT_EQ(lift.StageCount, 2u);
    EXPECT_EQ(lift.HasExit, 1u);
    EXPECT_EQ(lift.ExitTick, 1227u);
    EXPECT_EQ(lift.ExitBiped, 0xB1u);
    EXPECT_FLOAT_EQ(lift.ExitVelocity.X, 0.252f);
    EXPECT_FLOAT_EQ(lift.ExitVelocity.Y, -6.162f);
    EXPECT_FLOAT_EQ(lift.ExitVelocity.Z, 4.114f);
    EXPECT_FLOAT_EQ(lift.Stages[0].Position.Z, 3.0f);
    EXPECT_FLOAT_EQ(lift.Stages[0].Direction.Y, -0.82f);
    EXPECT_FLOAT_EQ(lift.Stages[0].Acceleration, 11.0f);
    EXPECT_FLOAT_EQ(lift.Stages[0].MaximumVelocity, 7.5f);
    EXPECT_FLOAT_EQ(lift.Stages[0].AxisAcceleration, 25.0f);
    EXPECT_FLOAT_EQ(lift.Stages[0].AxisMaximumVelocity, 20.0f);
    EXPECT_EQ(lift.Stages[0].Affects, 15u);
    EXPECT_FLOAT_EQ(lift.Stages[1].Direction.Z, 1.0f);
    EXPECT_EQ(lift.Stages[1].Affects, 3u);
    EXPECT_EQ(flat->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::LiftsTruncated), 0u);

    LiftDetails many{};
    for (std::uint32_t i{ 0 }; i < k_BridgeMaxLifts + 1; ++i) many[0x100 + i] = LiftDetail{ .Handle = 0x100 + i };
    many[0x100].Stages.assign(k_BridgeMaxLiftStages + 1, second);
    Tick full{};
    full.LiftDetails = std::make_shared<const LiftDetails>(std::move(many));
    const auto flatFull{ Flat(full) };
    EXPECT_EQ(flatFull->Header.LiftCount, k_BridgeMaxLifts);
    EXPECT_NE(flatFull->Header.Flags & static_cast<std::uint32_t>(BridgeTickFlag::LiftsTruncated), 0u);
    // A tag with more stages than the row holds keeps the first ones.
    EXPECT_EQ(flatFull->Lifts[0].StageCount, k_BridgeMaxLiftStages);
}

TEST(FlatTests, WithoutWeaponOrProjectileDetailsTheCountsAreZero)
{
    const auto flat{ Flat(Tick{}) };

    EXPECT_EQ(flat->Header.WeaponCount, 0u);
    EXPECT_EQ(flat->Header.ProjectileCount, 0u);
    EXPECT_EQ(flat->Header.ExplosiveCount, 0u);
    EXPECT_EQ(flat->Header.LiftCount, 0u);
}

TEST(FlatTests, MatchCarriesTheRoundAndTheScoresOfEachTeam)
{
    MatchStatus match{};
    match.GameTick = 4000;
    match.RoundIndex = 2;
    match.RoundState = RoundState::Between;
    match.TeamScores[0] = { .Round = 1, .Total = 3 };
    match.TeamScores[1] = { .Round = 0, .Total = 2 };
    match.TeamScores[7] = { .Round = -1, .Total = 9 };
    match.RoundClockTicks = 7200;
    match.RoundTicksRemaining = -1;
    match.RoundTimeLimitTicks = 36000;
    match.RoundLimit = 3;
    match.ScoreToWin = 5;
    match.RespawnTimeTicks = 300;
    match.LivesPerRound = 2;
    match.TeamLivesPerRound = 4;
    match.RoundsToWin = 3;
    match.SuddenDeathTicks = -1;
    match.GracePeriodTicks = 300;
    match.PowerupDurationTicks = { 360, 900, 5400 };

    Tick tick{};
    tick.Match = std::make_shared<const MatchStatus>(std::move(match));
    const auto flat{ Flat(tick) };

    EXPECT_EQ(flat->Match.GameTick, 4000u);
    EXPECT_EQ(flat->Match.RoundIndex, 2);
    EXPECT_EQ(flat->Match.RoundState, static_cast<std::uint8_t>(RoundState::Between));
    EXPECT_EQ(flat->Match.TeamRoundScores[0], 1);
    EXPECT_EQ(flat->Match.TeamTotalScores[0], 3);
    EXPECT_EQ(flat->Match.TeamRoundScores[1], 0);
    EXPECT_EQ(flat->Match.TeamTotalScores[1], 2);
    EXPECT_EQ(flat->Match.TeamRoundScores[7], -1);
    EXPECT_EQ(flat->Match.TeamTotalScores[7], 9);
    EXPECT_EQ(flat->Match.TeamRoundScores[2], 0);
    EXPECT_EQ(flat->Match.RoundClockTicks, 7200u);
    EXPECT_EQ(flat->Match.RoundTicksRemaining, -1);
    EXPECT_EQ(flat->Match.RoundTimeLimitTicks, 36000u);
    EXPECT_EQ(flat->Match.RoundLimit, 3);
    EXPECT_EQ(flat->Match.ScoreToWin, 5);
    EXPECT_EQ(flat->Match.RespawnTimeTicks, 300u);
    EXPECT_EQ(flat->Match.LivesPerRound, 2);
    EXPECT_EQ(flat->Match.TeamLivesPerRound, 4);
    EXPECT_EQ(flat->Match.RoundsToWin, 3);
    EXPECT_EQ(flat->Match.SuddenDeathTicks, -1);
    EXPECT_EQ(flat->Match.GracePeriodTicks, 300u);
    EXPECT_EQ(flat->Match.PowerupDurationTicks[0], 360u);
    EXPECT_EQ(flat->Match.PowerupDurationTicks[1], 900u);
    EXPECT_EQ(flat->Match.PowerupDurationTicks[2], 5400u);
}

TEST(FlatTests, HeaderCarriesTheMapId)
{
    Tick tick{};
    tick.MapId = 1234;
    EXPECT_EQ(Flat(tick)->Header.MapId, 1234);
    EXPECT_EQ(Flat(Tick{})->Header.MapId, 0);
}

TEST(FlatTests, RespawnTicksReachThePlayerRowWithAndWithoutBody)
{
    auto players{ std::make_shared<Players>() };
    players->Rows.push_back(Player{ .Handle = 0xE5A10001, .RespawnTicks = 420 });
    players->Rows.push_back(Player{ .Handle = 0xE5A10002, .RespawnTicks = 0, .Body = Body{} });
    Tick tick{};
    tick.Players = players;

    const auto flat{ Flat(tick) };
    ASSERT_EQ(flat->Header.PlayerCount, 2u);
    EXPECT_EQ(flat->Players[0].HasBody, 0);
    EXPECT_EQ(flat->Players[0].RespawnTicks, 420u);
    EXPECT_EQ(flat->Players[1].HasBody, 1);
    EXPECT_EQ(flat->Players[1].RespawnTicks, 0u);
}

TEST(FlatTests, PowerupReachesThePlayerRowWithAndWithoutBody)
{
    auto players{ std::make_shared<Players>() };
    players->Rows.push_back(Player{ .Handle = 0xE5A10001, .Powerup = Common::Semantic::Type::Subtype::Powerup::Red, .PowerupTicks = 532 });
    players->Rows.push_back(Player{ .Handle = 0xE5A10002, .Powerup = Common::Semantic::Type::Subtype::Powerup::Yellow, .PowerupTicks = 1, .Body = Body{} });
    players->Rows.push_back(Player{ .Handle = 0xE5A10003 });
    Tick tick{};
    tick.Players = players;

    const auto flat{ Flat(tick) };
    ASSERT_EQ(flat->Header.PlayerCount, 3u);
    EXPECT_EQ(flat->Players[0].HasBody, 0);
    EXPECT_EQ(flat->Players[0].Powerup, BridgePowerup::Red);
    EXPECT_EQ(flat->Players[0].PowerupTicks, 532u);
    EXPECT_EQ(flat->Players[1].HasBody, 1);
    EXPECT_EQ(flat->Players[1].Powerup, BridgePowerup::Yellow);
    EXPECT_EQ(flat->Players[1].PowerupTicks, 1u);
    EXPECT_EQ(flat->Players[2].Powerup, BridgePowerup::NoPowerup);
    EXPECT_EQ(flat->Players[2].PowerupTicks, 0u);
}

TEST(FlatTests, PlayerTraitsReachThePlayerRowWithAndWithoutBody)
{
    PlayerTraits traits{};
    traits.DamageResistance = std::numeric_limits<float>::infinity();
    traits.MeleeMultiplier = 1.25f;
    traits.MovementSpeed = 0.9f;
    traits.Gravity = 0.75f;
    traits.JumpHeight = 1.5f;
    traits.RadarRange = 75.0f;
    traits.IsHeadshotImmune = true;
    traits.AreAbilitiesInfinite = true;
    traits.InfiniteAmmo = Blam::Layout::Player::Type::Traits::InfiniteAmmoMode::Bottomless;
    traits.VehicleUse = Blam::Layout::Player::Type::Traits::VehicleUseMode::NoDriver;
    traits.VisibleWaypoint = Blam::Layout::Player::Type::Traits::IdentityVisibility::Everyone;
    traits.RadarMode = Blam::Layout::Player::Type::Traits::RadarDisplayMode::Enhanced;
    traits.FragGrenades = 2;
    traits.PlasmaGrenades = 255;
    traits.ActiveCamo = 3;
    traits.PrimaryWeapon = -1;
    traits.Ability = 4;

    auto players{ std::make_shared<Players>() };
    players->Rows.push_back(Player{ .Handle = 0xE5A10001, .Traits = traits });
    players->Rows.push_back(Player{ .Handle = 0xE5A10002, .Traits = traits, .Body = Body{} });
    Tick tick{};
    tick.Players = players;

    const auto flat{ Flat(tick) };
    ASSERT_EQ(flat->Header.PlayerCount, 2u);
    EXPECT_EQ(flat->Players[0].HasBody, 0);
    EXPECT_EQ(flat->Players[1].HasBody, 1);
    for (const auto& row : std::span{ flat->Players }.first(2))
    {
        EXPECT_TRUE(std::isinf(row.Traits.DamageResistance));
        EXPECT_FLOAT_EQ(row.Traits.MeleeMultiplier, 1.25f);
        EXPECT_FLOAT_EQ(row.Traits.MovementSpeed, 0.9f);
        EXPECT_FLOAT_EQ(row.Traits.Gravity, 0.75f);
        EXPECT_FLOAT_EQ(row.Traits.JumpHeight, 1.5f);
        EXPECT_FLOAT_EQ(row.Traits.RadarRange, 75.0f);
        EXPECT_EQ(row.Traits.HeadshotImmune, 1);
        EXPECT_EQ(row.Traits.AssassinationImmune, 0);
        EXPECT_EQ(row.Traits.InfiniteAbilities, 1);
        EXPECT_EQ(row.Traits.InfiniteAmmo, BridgeInfiniteAmmo::Bottomless);
        EXPECT_EQ(row.Traits.VehicleUse, BridgeVehicleUse::NoDriver);
        EXPECT_EQ(row.Traits.VisibleWaypoint, BridgeIdentityVisibility::Everyone);
        EXPECT_EQ(row.Traits.VisibleName, BridgeIdentityVisibility::Unknown);
        EXPECT_EQ(row.Traits.RadarMode, BridgeRadarMode::Enhanced);
        EXPECT_EQ(row.Traits.FragGrenades, 2);
        EXPECT_EQ(row.Traits.PlasmaGrenades, 255);
        EXPECT_EQ(row.Traits.ActiveCamo, 3);
        EXPECT_EQ(row.Traits.PrimaryWeapon, -1);
        EXPECT_EQ(row.Traits.Ability, 4);
    }
}

TEST(FlatTests, APlayerWithoutBodyHasNoHandleInTheHandlesOfTheBody)
{
    Body body{};
    body.Loadout.PrimaryWeapon = 0xE4010001;
    body.Loadout.SecondaryWeapon = 0xE4010002;
    body.Loadout.EquippedWeapon = 0xE4010001;
    body.Loadout.Ability = 0xE4010003;
    body.Loadout.Objective = 0xE4010004;
    body.LastDamagerPlayer = 0xE5A10009;
    body.GroundObject = 0xE4030194;
    body.Zoom = decltype(body.Zoom)::Zoom1;

    auto players{ std::make_shared<Players>() };
    players->Rows.push_back(Player{ .Handle = 0xE5A10001, .BipedHandle = 0xE4330001, .Body = body });
    players->Rows.push_back(Player{ .Handle = 0xE5A10002 });
    Tick tick{};
    tick.Players = players;

    const auto flat{ Flat(tick) };
    ASSERT_EQ(flat->Header.PlayerCount, 2u);
    const auto& with{ flat->Players[0] };
    EXPECT_EQ(with.PrimaryWeapon, 0xE4010001u);
    EXPECT_EQ(with.SecondaryWeapon, 0xE4010002u);
    EXPECT_EQ(with.EquippedWeapon, 0xE4010001u);
    EXPECT_EQ(with.Ability, 0xE4010003u);
    EXPECT_EQ(with.Objective, 0xE4010004u);
    EXPECT_EQ(with.Zoom, 0);
    EXPECT_EQ(with.LastDamagerPlayer, 0xE5A10009u);
    EXPECT_EQ(with.GroundObject, 0xE4030194u);

    const auto& without{ flat->Players[1] };
    EXPECT_EQ(without.HasBody, 0);
    EXPECT_EQ(without.BipedHandle, k_NoHandle);
    EXPECT_EQ(without.PrimaryWeapon, k_NoHandle);
    EXPECT_EQ(without.SecondaryWeapon, k_NoHandle);
    EXPECT_EQ(without.EquippedWeapon, k_NoHandle);
    EXPECT_EQ(without.Ability, k_NoHandle);
    EXPECT_EQ(without.Objective, k_NoHandle);
    EXPECT_EQ(without.LastDamagerPlayer, k_NoHandle);
    EXPECT_EQ(without.GroundObject, k_NoHandle);
    EXPECT_EQ(without.TicksSinceDamage, k_NoHandle);
    // No zoom (0xFF), not the 0 of Zoom1.
    EXPECT_EQ(without.Zoom, 0xFF);
}

TEST(FlatTests, GroundAndCrouchReachThePlayerRowAndAreZeroWithoutBody)
{
    Body body{};
    body.Ground = GroundKind::Object;
    body.IsGrounded = true;
    body.IsCrouching = true;
    body.AirborneTicks = 2;
    body.GroundObject = 0xE4030194;
    body.OwnVelocity = { 0.5f, -1.5f, 2.5f };
    body.CrouchFraction = 0.75f;

    auto players{ std::make_shared<Players>() };
    players->Rows.push_back(Player{ .Handle = 0xE5A10001, .Body = body });
    players->Rows.push_back(Player{ .Handle = 0xE5A10002 });
    Tick tick{};
    tick.Players = players;

    const auto flat{ Flat(tick) };
    ASSERT_EQ(flat->Header.PlayerCount, 2u);
    const auto& with{ flat->Players[0] };
    EXPECT_EQ(with.Ground, BridgeGround::Object);
    EXPECT_EQ(with.IsGrounded, 1);
    EXPECT_EQ(with.IsCrouching, 1);
    EXPECT_EQ(with.AirborneTicks, 2u);
    EXPECT_EQ(with.GroundObject, 0xE4030194u);
    EXPECT_FLOAT_EQ(with.OwnVelocity.X, 0.5f);
    EXPECT_FLOAT_EQ(with.OwnVelocity.Y, -1.5f);
    EXPECT_FLOAT_EQ(with.OwnVelocity.Z, 2.5f);
    EXPECT_FLOAT_EQ(with.CrouchFraction, 0.75f);

    const auto& without{ flat->Players[1] };
    EXPECT_EQ(without.Ground, BridgeGround::Air);
    EXPECT_EQ(without.IsGrounded, 0);
    EXPECT_EQ(without.IsCrouching, 0);
    EXPECT_EQ(without.AirborneTicks, 0u);
    EXPECT_EQ(without.GroundObject, k_NoHandle);
    EXPECT_FLOAT_EQ(without.OwnVelocity.Z, 0.0f);
    EXPECT_FLOAT_EQ(without.CrouchFraction, 0.0f);
}

TEST(FlatTests, EventsCarryTheSequenceOfArtemisAndTheOneOfTheEngine)
{
    EventLog log{};
    log.Rows.push_back(GameEvent{ .Sequence = 1, .EngineSequence = 57, .GameTick = 10, .Kind = Event::FlagTaken });
    log.Rows.push_back(GameEvent{ .Sequence = 2, .EngineSequence = 0, .GameTick = 11, .Kind = Event::FlagDropped });
    log.LastSequence = 2;

    Tick tick{};
    tick.Events = std::make_shared<const EventLog>(std::move(log));
    const auto flat{ Flat(tick) };

    ASSERT_EQ(flat->Header.EventCount, 2u);
    EXPECT_EQ(flat->Events[0].Sequence, 1u);
    EXPECT_EQ(flat->Events[0].EngineSequence, 57u);
    EXPECT_EQ(flat->Events[1].Sequence, 2u);
    EXPECT_EQ(flat->Events[1].EngineSequence, 0u);
    EXPECT_EQ(flat->Header.LastEventSequence, 2u);
}

TEST(FlatTests, ViewsAreCopiedRowByRow)
{
    auto views{ std::make_shared<ViewTable>() };
    FillView((*views)[0], 100);
    FillView((*views)[1], 5'000);
    (*views)[1].Head.Mode = ViewMode::Full;

    const auto flat{ Flat(WithViews(2, views)) };

    ASSERT_EQ(flat->Header.ViewCount, 2u);
    ExpectView((*views)[0], flat->Views[0]);

    // The second row is the second player: the same View the table has in that row, with the mode it carries.
    EXPECT_EQ(flat->Views[1].Head.PlayerHandle, (*views)[1].Head.PlayerHandle);
    EXPECT_EQ(flat->Views[1].Head.Mode, BridgeViewMode::Full);
    EXPECT_EQ(flat->Views[1].Self.Shield, (*views)[1].Self.Shield);
    EXPECT_EQ(flat->Views[1].Others[7].Shield, (*views)[1].Others[7].Shield);
    EXPECT_NE(flat->Views[0].Head.PlayerHandle, flat->Views[1].Head.PlayerHandle);
}

TEST(FlatTests, ViewMaskAndCountFollowComputedRows)
{
    auto views{ std::make_shared<ViewTable>() };
    (*views)[0].Head.IsComputed = true;
    (*views)[2].Head.IsComputed = true;
    (*views)[5].Head.IsComputed = true;

    // The mask is by row of Players: row 5 is outside the three players of the tick, so it is not counted.
    const auto flat{ Flat(WithViews(3, views)) };

    EXPECT_EQ(flat->Header.ViewCount, 3u);
    EXPECT_EQ(flat->Header.ViewMask, 0b101u);

    // Without a Views table the count follows the players and nothing is computed.
    const auto without{ Flat(WithViews(3, nullptr)) };
    EXPECT_EQ(without->Header.ViewCount, 3u);
    EXPECT_EQ(without->Header.ViewMask, 0u);
    EXPECT_EQ(Flat(Tick{})->Header.ViewCount, 0u);
}

TEST(FlatTests, RowsOutsideTheSetAreZero)
{
    auto views{ std::make_shared<ViewTable>() };
    FillView((*views)[0], 10);
    FillView((*views)[1], 20);
    FillView((*views)[2], 30);

    // A buffer that held a bigger tick before: whatever was in the rows past the players, and in the padding, must not survive.
    auto flat{ std::make_unique<BridgeTick>() };
    std::memset(flat->Views.data(), 0xAB, sizeof(flat->Views));
    FlatService{}.Flatten(WithViews(1, views), *flat);

    ASSERT_EQ(flat->Header.ViewCount, 1u);
    const BridgeView zero{ [] { BridgeView value{}; std::memset(&value, 0, sizeof(value)); return value; }() };
    for (std::size_t row{ 1 }; row < flat->Views.size(); ++row)
    {
        EXPECT_EQ(std::memcmp(&flat->Views[row], &zero, sizeof(BridgeView)), 0) << "row " << row;
    }

    // A computed row with every field zero is zero bytes too, so the padding of the rows in use is clean.
    auto empty{ std::make_shared<ViewTable>() };
    auto dirty{ std::make_unique<BridgeTick>() };
    std::memset(dirty->Views.data(), 0xAB, sizeof(dirty->Views));
    FlatService{}.Flatten(WithViews(2, empty), *dirty);
    EXPECT_EQ(std::memcmp(&dirty->Views[0], &zero, sizeof(BridgeView)), 0);
    EXPECT_EQ(std::memcmp(&dirty->Views[1], &zero, sizeof(BridgeView)), 0);
}

TEST(FlatTests, EventPositionAndAmountAreZero)
{
    EventLog log{};
    log.Rows.push_back(GameEvent{ .Sequence = 1, .EngineSequence = 57, .GameTick = 10, .Kind = Event::FlagTaken });
    log.LastSequence = 1;
    Tick tick{};
    tick.Events = std::make_shared<const EventLog>(std::move(log));

    // Perception has no position or amount yet (F4.5): the fields cross as zero even over a buffer that was not.
    auto flat{ std::make_unique<BridgeTick>() };
    std::memset(flat->Events.data(), 0xAB, sizeof(flat->Events));
    FlatService{}.Flatten(tick, *flat);

    ASSERT_EQ(flat->Header.EventCount, 1u);
    EXPECT_EQ(flat->Events[0].Sequence, 1u);
    EXPECT_EQ(flat->Events[0].Position.X, 0.0f);
    EXPECT_EQ(flat->Events[0].Position.Y, 0.0f);
    EXPECT_EQ(flat->Events[0].Position.Z, 0.0f);
    EXPECT_EQ(flat->Events[0].Amount, 0.0f);
}