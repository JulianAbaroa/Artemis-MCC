module Bridge.Codec.Flat.System;

import Common.Engine.Type;
import Common.Math.Type;
import Blam.Layout.Player.Type;
import Blam.Runtime.Object.Type;
import Perception.Entities.Type;
import Perception.Players.Type;
import Perception.Details.Type;
import Perception.Events.Type;
import Perception.Match.Type;
import Views.Raycast.Type;
import Views.View.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Perception::Entities::Type::Entity;
    using Perception::Players::Type::Player;
    using Perception::Events::Type::GameEvent;
    using Perception::Match::Type::MatchScore;
    using Perception::Details::Type::ExplosiveDetail;
    using Perception::Details::Type::LiftDetail;
    using Perception::Details::Type::LiftStage;
    using Perception::Details::Type::ObjectiveDetail;
    using Perception::Details::Type::ProjectileDetail;
    using Perception::Details::Type::StandDetail;
    using Perception::Details::Type::WeaponDetail;
    using Blam::Layout::Player::Type::Traits::PlayerTraits;
    using Blam::Runtime::Object::Type::Biped::ZoomLevel;
    using Views::Raycast::Type::HitKind;
    using Views::View::Type::View;
    using Views::View::Type::ViewAimHit;
    using Views::View::Type::ViewChannel;
    using Views::View::Type::ViewFlag;
    using Views::View::Type::ViewFlagSide;
    using Views::View::Type::ViewHead;
    using Views::View::Type::ViewMode;
    using Views::View::Type::ViewNotice;
    using Views::View::Type::ViewOther;
    using Views::View::Type::ViewRayHit;
    using Views::View::Type::ViewRelation;
    using Views::View::Type::ViewSelf;
    using Views::View::Type::ViewSound;
    using Views::View::Type::ViewSoundKind;
    using Views::View::Type::ViewWeapon;
    using Contract::Bridge::Type::BridgeAgentInput;
    using Contract::Bridge::Type::BridgeHitKind;
    using Contract::Bridge::Type::BridgeView;
    using Contract::Bridge::Type::BridgeViewAimHit;
    using Contract::Bridge::Type::BridgeViewChannel;
    using Contract::Bridge::Type::BridgeViewFlag;
    using Contract::Bridge::Type::BridgeViewFlagSide;
    using Contract::Bridge::Type::BridgeViewHead;
    using Contract::Bridge::Type::BridgeViewMode;
    using Contract::Bridge::Type::BridgeViewNotice;
    using Contract::Bridge::Type::BridgeViewOther;
    using Contract::Bridge::Type::BridgeViewRay;
    using Contract::Bridge::Type::BridgeViewRelation;
    using Contract::Bridge::Type::BridgeViewSelf;
    using Contract::Bridge::Type::BridgeViewSound;
    using Contract::Bridge::Type::BridgeViewSoundKind;
    using Contract::Bridge::Type::BridgeViewWeapon;
    using Contract::Bridge::Type::BridgeGround;
    using Contract::Bridge::Type::BridgePlayerTraits;
    using Contract::Bridge::Type::BridgeInfiniteAmmo;
    using Contract::Bridge::Type::BridgeAbilityUsage;
    using Contract::Bridge::Type::BridgeVehicleUse;
    using Contract::Bridge::Type::BridgeIdentityVisibility;
    using Contract::Bridge::Type::BridgeRadarMode;
    using Contract::Bridge::Type::BridgePowerup;
    using Contract::Bridge::Type::BridgeVec3;
    using Contract::Bridge::Type::BridgePlayer;
    using Contract::Bridge::Type::BridgeEntity;
    using Contract::Bridge::Type::BridgeMatch;
    using Contract::Bridge::Type::BridgeEvent;
    using Contract::Bridge::Type::BridgeExplosive;
    using Contract::Bridge::Type::BridgeLift;
    using Contract::Bridge::Type::BridgeLiftStage;
    using Contract::Bridge::Type::BridgeObjective;
    using Contract::Bridge::Type::BridgeProjectile;
    using Contract::Bridge::Type::BridgeStand;
    using Contract::Bridge::Type::BridgeWeapon;
    using Contract::Bridge::Type::BridgeTickFlag;

    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_HandleIndexMask;
    using Contract::Bridge::Type::k_BridgeNotLocal;
    using Contract::Bridge::Type::k_BridgeMaxLiftStages;
    using Contract::Bridge::Type::k_BridgeViewFlags;
    using Contract::Bridge::Type::k_BridgeViewNotices;
    using Contract::Bridge::Type::k_BridgeViewRayCount;
    using Contract::Bridge::Type::k_BridgeViewSounds;
    using Contract::Bridge::Type::k_BridgeViewWeaponSlots;
    using Views::View::Type::k_ViewFlags;
    using Views::View::Type::k_ViewNotices;
    using Views::View::Type::k_ViewRayCount;
    using Views::View::Type::k_ViewSounds;
    using Views::View::Type::k_ViewWeaponSlots;

    // The constants and the values of the enums of Bridge.xml are copies of the ones of Views: a View is copied field by field and
    // its enums are cast, so a drift has to fail here.
    static_assert(k_BridgeViewRayCount == k_ViewRayCount && k_BridgeViewFlags == k_ViewFlags && k_BridgeViewSounds == k_ViewSounds &&
        k_BridgeViewNotices == k_ViewNotices && k_BridgeViewWeaponSlots == k_ViewWeaponSlots);
    static_assert(std::to_underlying(BridgeViewMode::Full) == std::to_underlying(ViewMode::Full) &&
        std::to_underlying(BridgeViewMode::HumanLike) == std::to_underlying(ViewMode::HumanLike));
    static_assert(std::to_underlying(BridgeViewRelation::NoRelation) == std::to_underlying(ViewRelation::None) &&
        std::to_underlying(BridgeViewRelation::Self) == std::to_underlying(ViewRelation::Self) &&
        std::to_underlying(BridgeViewRelation::Ally) == std::to_underlying(ViewRelation::Ally) &&
        std::to_underlying(BridgeViewRelation::Enemy) == std::to_underlying(ViewRelation::Enemy) &&
        std::to_underlying(BridgeViewRelation::Neutral) == std::to_underlying(ViewRelation::Neutral));
    static_assert(static_cast<std::uint8_t>(BridgeViewChannel::Hud) == std::to_underlying(ViewChannel::Hud) &&
        static_cast<std::uint8_t>(BridgeViewChannel::Seen) == std::to_underlying(ViewChannel::Seen) &&
        static_cast<std::uint8_t>(BridgeViewChannel::Radar) == std::to_underlying(ViewChannel::Radar));
    static_assert(std::to_underlying(BridgeViewSoundKind::Fire) == std::to_underlying(ViewSoundKind::Fire) &&
        std::to_underlying(BridgeViewSoundKind::Explosion) == std::to_underlying(ViewSoundKind::Explosion));
    static_assert(std::to_underlying(BridgeViewFlagSide::NoSide) == std::to_underlying(ViewFlagSide::None) &&
        std::to_underlying(BridgeViewFlagSide::Mine) == std::to_underlying(ViewFlagSide::Mine) &&
        std::to_underlying(BridgeViewFlagSide::Rival) == std::to_underlying(ViewFlagSide::Rival));
    static_assert(std::to_underlying(BridgeHitKind::Miss) == std::to_underlying(HitKind::None) &&
        std::to_underlying(BridgeHitKind::Static) == std::to_underlying(HitKind::Static) &&
        std::to_underlying(BridgeHitKind::Dynamic) == std::to_underlying(HitKind::Dynamic));

    // Value of an optional float that is absent (Shield, Vitality).
    constexpr float k_Absent{ std::numeric_limits<float>::quiet_NaN() };

    // return: A row with every byte zero, its padding included.
    // note: The rows are compared and written byte by byte (TickDump delta, the offline test, D208), so the padding cannot carry
    // what was on the stack. Copies of these trivially copyable rows keep the zeros.
    template<typename T>
        requires std::is_trivially_copyable_v<T>
    auto Zeroed() -> T
    {
        T value{};
        std::memset(&value, 0, sizeof(T));
        return value;
    }

    auto ToBridge(const Vec3& v) -> BridgeVec3
    {
        return BridgeVec3{ .X = v.X, .Y = v.Y, .Z = v.Z };
    }

    auto ToBridge(const PlayerTraits& traits) -> BridgePlayerTraits
    {
        BridgePlayerTraits row{ Zeroed<BridgePlayerTraits>() };
        row.DamageResistance = traits.DamageResistance;
        row.HealthMultiplier = traits.HealthMultiplier;
        row.HealthRegenRate = traits.HealthRegenRate;
        row.ShieldMultiplier = traits.ShieldMultiplier;
        row.ShieldRegenRate = traits.ShieldRegenRate;
        row.OvershieldRegenRate = traits.OvershieldRegenRate;
        row.ShieldVampirism = traits.ShieldVampirism;
        row.DamageMultiplier = traits.DamageMultiplier;
        row.MeleeMultiplier = traits.MeleeMultiplier;
        row.MovementSpeed = traits.MovementSpeed;
        row.Gravity = traits.Gravity;
        row.JumpHeight = traits.JumpHeight;
        row.RadarRange = traits.RadarRange;
        row.HeadshotImmune = traits.IsHeadshotImmune ? 1 : 0;
        row.AssassinationImmune = traits.IsAssassinationImmune ? 1 : 0;
        row.CannotDieFromDamage = traits.CannotDieFromDamage ? 1 : 0;
        row.GrenadeRegen = traits.IsGrenadeRegenOn ? 1 : 0;
        row.WeaponPickup = traits.IsWeaponPickupOn ? 1 : 0;
        row.AbilitiesDropOnDeath = traits.DoAbilitiesDropOnDeath ? 1 : 0;
        row.InfiniteAbilities = traits.AreAbilitiesInfinite ? 1 : 0;
        row.InfiniteAmmo = static_cast<BridgeInfiniteAmmo>(traits.InfiniteAmmo);
        row.AbilityUsage = static_cast<BridgeAbilityUsage>(traits.AbilityUsage);
        row.VehicleUse = static_cast<BridgeVehicleUse>(traits.VehicleUse);
        row.VisibleWaypoint = static_cast<BridgeIdentityVisibility>(traits.VisibleWaypoint);
        row.VisibleName = static_cast<BridgeIdentityVisibility>(traits.VisibleName);
        row.RadarMode = static_cast<BridgeRadarMode>(traits.RadarMode);
        row.FragGrenades = traits.FragGrenades;
        row.PlasmaGrenades = traits.PlasmaGrenades;
        row.ActiveCamo = traits.ActiveCamo;
        row.PrimaryWeapon = traits.PrimaryWeapon;
        row.SecondaryWeapon = traits.SecondaryWeapon;
        row.Ability = traits.Ability;
        return row;
    }

    auto ToBridge(const Player& player) -> BridgePlayer
    {
        BridgePlayer row{ Zeroed<BridgePlayer>() };
        row.Handle = player.Handle;
        row.BipedHandle = player.BipedHandle;
        row.RespawnTicks = player.RespawnTicks;
        row.Traits = ToBridge(player.Traits);
        row.Powerup = static_cast<BridgePowerup>(player.Powerup);
        row.PowerupTicks = player.PowerupTicks;
        row.Team = static_cast<std::uint8_t>(player.Team);
        row.LocalIndex = player.LocalIndex.value_or(k_BridgeNotLocal);
        row.Connection = static_cast<std::uint8_t>(player.Connection);
        row.IsAlive = player.IsAlive() ? 1 : 0;
        row.CameraPosition = ToBridge(player.CameraPosition);
        row.CameraForward = ToBridge(player.CameraForward);
        row.TicksSinceDamage = k_NoHandle;

        // Without a body the handles of the body do not exist (NoHandle, not the zero of Zeroed) and there is no zoom (0xFF, not the 0 of Zoom1).
        row.PrimaryWeapon = k_NoHandle;
        row.SecondaryWeapon = k_NoHandle;
        row.EquippedWeapon = k_NoHandle;
        row.Ability = k_NoHandle;
        row.Objective = k_NoHandle;
        row.LastDamagerPlayer = k_NoHandle;
        row.GroundObject = k_NoHandle;
        row.Zoom = static_cast<std::uint8_t>(ZoomLevel::None);

        if (!player.Body) return row;

        const auto& body{ *player.Body };
        row.HasBody = 1;
        row.Zoom = static_cast<std::uint8_t>(body.Zoom);
        row.SelectedGrenade = static_cast<std::uint8_t>(body.Loadout.SelectedGrenade);
        row.FragGrenades = body.Loadout.FragGrenades;
        row.PlasmaGrenades = body.Loadout.PlasmaGrenades;
        row.IsAbilityActive = body.Loadout.IsAbilityActive ? 1 : 0;
        row.Aim = ToBridge(body.Aim);
        row.TicksAlive = body.TicksAlive;
        row.Shield = body.Shield.value_or(k_Absent);
        row.PrimaryWeapon = body.Loadout.PrimaryWeapon;
        row.SecondaryWeapon = body.Loadout.SecondaryWeapon;
        row.EquippedWeapon = body.Loadout.EquippedWeapon;
        row.Ability = body.Loadout.Ability;
        row.AbilityEnergy = body.Loadout.AbilityEnergy;
        row.Objective = body.Loadout.Objective;
        row.LastDamagerPlayer = body.LastDamagerPlayer;
        row.TicksSinceDamage = body.TicksSinceDamage.value_or(k_NoHandle);
        row.Ground = static_cast<BridgeGround>(body.Ground);
        row.IsGrounded = body.IsGrounded ? 1 : 0;
        row.IsCrouching = body.IsCrouching ? 1 : 0;
        row.AirborneTicks = body.AirborneTicks;
        row.GroundObject = body.GroundObject;
        row.CrouchFraction = body.CrouchFraction;
        row.OwnVelocity = ToBridge(body.OwnVelocity);
        return row;
    }

    auto ToBridge(const Entity& entity) -> BridgeEntity
    {
        BridgeEntity row{ Zeroed<BridgeEntity>() };
        row.Handle = entity.Handle;
        row.Kind = static_cast<std::uint16_t>(entity.Kind);
        row.Function = static_cast<std::uint8_t>(entity.Function);
        row.Subtype = entity.Subtype;
        row.Group = static_cast<std::uint8_t>(entity.Group);
        row.Lifecycle = static_cast<std::uint8_t>(entity.Lifecycle);
        row.Binding = static_cast<std::uint8_t>(entity.Binding);
        row.Team = static_cast<std::uint8_t>(entity.Team);
        row.Physics = static_cast<std::uint8_t>(entity.Physics);
        row.Capabilities = static_cast<std::uint16_t>(entity.Capabilities);
        row.BoundTo = entity.BoundTo;
        row.ParentHandle = entity.ParentHandle;
        row.BoundingRadius = entity.BoundingRadius;
        row.Vitality = entity.Vitality.value_or(k_Absent);
        row.BoundingCenter = ToBridge(entity.BoundingCenter);
        row.WorldOrigin = ToBridge(entity.WorldOrigin);
        row.Forward = ToBridge(entity.Forward);
        row.Up = ToBridge(entity.Up);
        row.LinearVelocity = ToBridge(entity.LinearVelocity);
        row.AngularVelocity = ToBridge(entity.AngularVelocity);
        return row;
    }

    auto ToBridge(const ObjectiveDetail& flag) -> BridgeObjective
    {
        BridgeObjective row{ Zeroed<BridgeObjective>() };
        row.Handle = flag.Handle;
        row.CarrierPlayer = flag.CarrierPlayer;
        row.CarrierBiped = flag.CarrierBiped;
        row.HomeStand = flag.HomeStand;
        row.ResetTicks = flag.ResetTicks;
        row.BaseTeam = static_cast<std::uint8_t>(flag.BaseTeam);
        row.OwnerTeam = static_cast<std::uint8_t>(flag.OwnerTeam);
        row.State = static_cast<std::uint8_t>(flag.State);
        row.IsResetRunning = flag.IsResetRunning ? 1 : 0;
        row.AlliesInZone = flag.AlliesInZone;
        return row;
    }

    auto ToBridge(const StandDetail& stand) -> BridgeStand
    {
        BridgeStand row{ Zeroed<BridgeStand>() };
        row.Handle = stand.Handle;
        row.BaseTeam = static_cast<std::uint8_t>(stand.BaseTeam);
        row.OwnerTeam = static_cast<std::uint8_t>(stand.OwnerTeam);
        return row;
    }

    auto ToBridge(const WeaponDetail& weapon) -> BridgeWeapon
    {
        BridgeWeapon row{ Zeroed<BridgeWeapon>() };
        row.Handle = weapon.Handle;
        row.TrackedObject = weapon.TrackedObject;
        row.Energy = weapon.Energy;
        row.Heat = weapon.Heat;
        row.ChargeFraction = weapon.ChargeFraction;
        for (std::size_t magazine{ 0 }; magazine < weapon.Magazines.size(); ++magazine)
        {
            row.Magazines[magazine].Loaded = weapon.Magazines[magazine].Loaded;
            row.Magazines[magazine].Reserve = weapon.Magazines[magazine].Reserve;
        }
        row.MagazineCount = weapon.MagazineCount;
        row.IsEnergyWeapon = weapon.IsEnergyWeapon ? 1 : 0;
        row.IsOverheated = weapon.IsOverheated ? 1 : 0;
        row.IsReloading = weapon.IsReloading ? 1 : 0;
        row.ChargeState = static_cast<std::uint8_t>(weapon.ChargeState);
        row.IsLocked = weapon.IsLocked ? 1 : 0;
        row.CanFireNow = weapon.CanFireNow ? 1 : 0;
        return row;
    }

    auto ToBridge(const ProjectileDetail& projectile) -> BridgeProjectile
    {
        BridgeProjectile row{ Zeroed<BridgeProjectile>() };
        row.Handle = projectile.Handle;
        row.OwnerPlayer = projectile.OwnerPlayer;
        row.OwnerBiped = projectile.OwnerBiped;
        row.ShooterRoot = projectile.ShooterRoot;
        row.StuckTo = projectile.StuckTo;
        row.StuckNode = projectile.StuckNode;
        row.InHand = projectile.InHand ? 1 : 0;
        return row;
    }

    auto ToBridge(const ExplosiveDetail& explosive) -> BridgeExplosive
    {
        BridgeExplosive row{ Zeroed<BridgeExplosive>() };
        row.Handle = explosive.Handle;
        row.RadiusMin = explosive.RadiusMin;
        row.RadiusMax = explosive.RadiusMax;
        row.Damage = explosive.Damage;
        return row;
    }

    auto ToBridge(const LiftStage& stage) -> BridgeLiftStage
    {
        BridgeLiftStage row{ Zeroed<BridgeLiftStage>() };
        row.Position = ToBridge(stage.Position);
        row.Direction = ToBridge(stage.Direction);
        row.Acceleration = stage.Acceleration;
        row.MaximumVelocity = stage.MaximumVelocity;
        row.AxisAcceleration = stage.AxisAcceleration;
        row.AxisMaximumVelocity = stage.AxisMaximumVelocity;
        row.Affects = stage.Affects;
        return row;
    }

    // note: A tag with more stages than the row holds loses the last ones (the tags have 3 at most).
    auto ToBridge(const LiftDetail& lift) -> BridgeLift
    {
        BridgeLift row{ Zeroed<BridgeLift>() };
        row.Handle = lift.Handle;
        row.StageCount = static_cast<std::uint8_t>((std::min)(lift.Stages.size(), static_cast<std::size_t>(k_BridgeMaxLiftStages)));
        row.HasExit = lift.HasExit ? 1 : 0;
        row.ExitTick = lift.ExitTick;
        row.ExitBiped = lift.ExitBiped;
        row.ExitVelocity = ToBridge(lift.ExitVelocity);
        for (std::size_t i{ 0 }; i < row.StageCount; ++i) row.Stages[i] = ToBridge(lift.Stages[i]);
        return row;
    }

    // return: Rows written, sorted by handle. Stops at the capacity of the array.
    template<typename Detail, typename Row, std::size_t N>
    auto FlattenByHandle(const std::unordered_map<std::uint32_t, Detail>& details, std::array<Row, N>& rows) -> std::size_t
    {
        std::vector<const Detail*> sorted{};
        sorted.reserve(details.size());
        for (const auto& [handle, detail] : details) sorted.push_back(&detail);
        std::ranges::sort(sorted, [](const Detail* a, const Detail* b) { return a->Handle < b->Handle; });

        const std::size_t count{ (std::min)(sorted.size(), N) };
        for (std::size_t i{ 0 }; i < count; ++i) rows[i] = ToBridge(*sorted[i]);
        return count;
    }

    // return: Rows written. They are the ones of a by-handle map in the order of the bridge: by BaseTeam, then by handle, so the same
    // flag keeps its slot while the engine recreates it. Stops at the capacity of the array.
    template<typename Detail, typename Row, std::size_t N>
    auto FlattenObjectives(const std::unordered_map<std::uint32_t, Detail>& details, std::array<Row, N>& rows) -> std::size_t
    {
        std::vector<const Detail*> sorted{};
        sorted.reserve(details.size());
        for (const auto& [handle, detail] : details) sorted.push_back(&detail);
        std::ranges::sort(sorted, [](const Detail* a, const Detail* b)
        {
            return a->BaseTeam != b->BaseTeam ? a->BaseTeam < b->BaseTeam : a->Handle < b->Handle;
        });

        const std::size_t count{ (std::min)(sorted.size(), N) };
        for (std::size_t i{ 0 }; i < count; ++i) rows[i] = ToBridge(*sorted[i]);
        return count;
    }

    auto ToBridge(const ViewHead& head) -> BridgeViewHead
    {
        BridgeViewHead row{ Zeroed<BridgeViewHead>() };
        row.PlayerHandle = head.PlayerHandle;
        row.PlayerRow = head.PlayerRow;
        row.LocalIndex = head.LocalIndex;
        row.BipedHandle = head.BipedHandle;
        row.Mode = static_cast<BridgeViewMode>(head.Mode);
        row.IsComputed = head.IsComputed ? 1 : 0;
        row.HasBody = head.HasBody ? 1 : 0;
        row.Team = static_cast<std::uint8_t>(head.Team);
        row.Side = static_cast<std::uint8_t>(head.Side);
        return row;
    }

    auto ToBridge(const ViewWeapon& weapon) -> BridgeViewWeapon
    {
        BridgeViewWeapon row{ Zeroed<BridgeViewWeapon>() };
        row.Handle = weapon.Handle;
        row.Subtype = weapon.Subtype;
        row.IsEquipped = weapon.IsEquipped ? 1 : 0;
        row.Loaded = weapon.Loaded;
        row.Reserve = weapon.Reserve;
        row.Energy = weapon.Energy;
        row.Heat = weapon.Heat;
        row.IsOverheated = weapon.IsOverheated ? 1 : 0;
        row.IsReloading = weapon.IsReloading ? 1 : 0;
        row.ChargeFraction = weapon.ChargeFraction;
        row.CanFireNow = weapon.CanFireNow ? 1 : 0;
        return row;
    }

    // note: Vitality and Shield are copied as they are, NaN included.
    auto ToBridge(const ViewSelf& self) -> BridgeViewSelf
    {
        BridgeViewSelf row{ Zeroed<BridgeViewSelf>() };
        row.Eye = ToBridge(self.Eye);
        row.Feet = ToBridge(self.Feet);
        row.FrameYaw = self.FrameYaw;
        row.AimPitch = self.AimPitch;
        row.LinearVelocity = ToBridge(self.LinearVelocity);
        row.OwnVelocity = ToBridge(self.OwnVelocity);
        row.Vitality = self.Vitality;
        row.Shield = self.Shield;
        row.Ground = static_cast<BridgeGround>(self.Ground);
        row.IsGrounded = self.IsGrounded ? 1 : 0;
        row.AirborneTicks = self.AirborneTicks;
        row.CrouchFraction = self.CrouchFraction;
        row.Zoom = static_cast<std::uint8_t>(self.Zoom);
        row.TicksAlive = self.TicksAlive;
        row.RespawnTicks = self.RespawnTicks;
        row.Powerup = static_cast<BridgePowerup>(self.Powerup);
        row.PowerupTicks = self.PowerupTicks;
        row.SelectedGrenade = static_cast<std::uint8_t>(self.SelectedGrenade);
        row.FragGrenades = self.FragGrenades;
        row.PlasmaGrenades = self.PlasmaGrenades;
        row.Ability = self.Ability;
        row.AbilityEnergy = self.AbilityEnergy;
        row.IsAbilityActive = self.IsAbilityActive ? 1 : 0;
        row.CarriesFlag = self.CarriesFlag ? 1 : 0;
        row.TicksSinceDamage = self.TicksSinceDamage;
        row.IsTracked = self.IsTracked ? 1 : 0;
        for (std::size_t i{ 0 }; i < self.Weapons.size(); ++i) row.Weapons[i] = ToBridge(self.Weapons[i]);
        return row;
    }

    auto ToBridge(const ViewAimHit& hit) -> BridgeViewAimHit
    {
        BridgeViewAimHit row{ Zeroed<BridgeViewAimHit>() };
        row.Distance = hit.Distance;
        row.MaxDistance = hit.MaxDistance;
        row.Point = ToBridge(hit.Point);
        row.Material = hit.Material;
        row.Kind = static_cast<BridgeHitKind>(hit.Kind);
        row.ObjectHandle = hit.ObjectHandle;
        row.Function = static_cast<std::uint8_t>(hit.Function);
        row.Subtype = hit.Subtype;
        row.Relation = static_cast<BridgeViewRelation>(hit.Relation);
        row.PlayerRow = hit.PlayerRow;
        row.RegionIndex = hit.RegionIndex;
        row.IsHead = hit.IsHead ? 1 : 0;
        row.SurfaceFlags = hit.SurfaceFlags;
        return row;
    }

    auto ToBridge(const ViewRayHit& ray) -> BridgeViewRay
    {
        BridgeViewRay row{ Zeroed<BridgeViewRay>() };
        row.Distance = ray.Distance;
        row.Kind = static_cast<BridgeHitKind>(ray.Kind);
        row.ObjectHandle = ray.ObjectHandle;
        row.Material = ray.Material;
        row.Function = static_cast<std::uint8_t>(ray.Function);
        row.Relation = static_cast<BridgeViewRelation>(ray.Relation);
        row.SurfaceFlags = ray.SurfaceFlags;
        return row;
    }

    auto ToBridge(const ViewOther& other) -> BridgeViewOther
    {
        BridgeViewOther row{ Zeroed<BridgeViewOther>() };
        row.Relation = static_cast<BridgeViewRelation>(other.Relation);
        row.Channels = other.Channels;
        row.IsAlive = other.IsAlive ? 1 : 0;
        row.Position = ToBridge(other.Position);
        row.Velocity = ToBridge(other.Velocity);
        row.FacingYaw = other.FacingYaw;
        row.Vitality = other.Vitality;
        row.Shield = other.Shield;
        row.WeaponSubtype = other.WeaponSubtype;
        row.IsCrouching = other.IsCrouching ? 1 : 0;
        row.CarriesFlag = other.CarriesFlag ? 1 : 0;
        row.Elevation = other.Elevation;
        return row;
    }

    auto ToBridge(const ViewFlag& flag) -> BridgeViewFlag
    {
        BridgeViewFlag row{ Zeroed<BridgeViewFlag>() };
        row.State = static_cast<std::uint8_t>(flag.State);
        row.Position = ToBridge(flag.Position);
        row.CarrierRow = flag.CarrierRow;
        row.HasFlag = flag.HasFlag ? 1 : 0;
        row.ResetTicks = flag.ResetTicks;
        row.IsResetRunning = flag.IsResetRunning ? 1 : 0;
        row.AlliesInZone = flag.AlliesInZone;
        row.StandPosition = ToBridge(flag.StandPosition);
        return row;
    }

    auto ToBridge(const ViewSound& sound) -> BridgeViewSound
    {
        BridgeViewSound row{ Zeroed<BridgeViewSound>() };
        row.Kind = static_cast<BridgeViewSoundKind>(sound.Kind);
        row.SourceFunction = static_cast<std::uint8_t>(sound.SourceFunction);
        row.SourceSubtype = sound.SourceSubtype;
        row.Direction = ToBridge(sound.Direction);
        row.Intensity = sound.Intensity;
        row.AgeTicks = sound.AgeTicks;
        return row;
    }

    auto ToBridge(const ViewNotice& notice) -> BridgeViewNotice
    {
        BridgeViewNotice row{ Zeroed<BridgeViewNotice>() };
        row.Kind = static_cast<std::uint8_t>(notice.Kind);
        row.CauseRow = notice.CauseRow;
        row.EffectRow = notice.EffectRow;
        row.CauseRelation = static_cast<BridgeViewRelation>(notice.CauseRelation);
        row.EffectRelation = static_cast<BridgeViewRelation>(notice.EffectRelation);
        row.FlagSide = static_cast<BridgeViewFlagSide>(notice.FlagSide);
        row.AgeTicks = notice.AgeTicks;
        return row;
    }

    auto ToBridge(const View& view) -> BridgeView
    {
        BridgeView row{ Zeroed<BridgeView>() };
        row.Head = ToBridge(view.Head);
        row.Self = ToBridge(view.Self);
        row.AimHit = ToBridge(view.AimHit);
        row.RayOrigin = ToBridge(view.RayOrigin);
        for (std::size_t i{ 0 }; i < view.Rays.size(); ++i) row.Rays[i] = ToBridge(view.Rays[i]);
        for (std::size_t i{ 0 }; i < view.Others.size(); ++i) row.Others[i] = ToBridge(view.Others[i]);
        for (std::size_t i{ 0 }; i < view.Flags.size(); ++i) row.Flags[i] = ToBridge(view.Flags[i]);
        row.SoundCount = view.SoundCount;
        for (std::size_t i{ 0 }; i < view.Sounds.size(); ++i) row.Sounds[i] = ToBridge(view.Sounds[i]);
        row.NoticeCount = view.NoticeCount;
        for (std::size_t i{ 0 }; i < view.Notices.size(); ++i) row.Notices[i] = ToBridge(view.Notices[i]);
        return row;
    }

    auto ToBridge(const GameEvent& event) -> BridgeEvent
    {
        BridgeEvent row{ Zeroed<BridgeEvent>() };
        row.Sequence = event.Sequence;
        row.EngineSequence = event.EngineSequence;
        row.GameTick = event.GameTick;
        row.Key = event.Key;
        row.CausePlayer = event.CausePlayer;
        row.CauseObject = event.CauseObject;
        row.EffectPlayer = event.EffectPlayer;
        row.EffectObject = event.EffectObject;
        row.Kind = static_cast<std::uint8_t>(event.Kind);
        row.DamageReportingType = event.DamageReportingType;
        row.IsHeadshot = event.IsHeadshot ? 1 : 0;
        row.IsStuck = event.IsStuck ? 1 : 0;
        return row;
    }
}

namespace Bridge::Codec::Flat::System
{
    auto FlatService::Flatten(const Tick& tick, BridgeTick& flat) const -> void
    {
        std::uint32_t flags{ 0 };

        std::size_t playerCount{ 0 };
        if (tick.Players)
        {
            for (const Player& player : tick.Players->Rows)
            {
                if (playerCount == flat.Players.size()) break;
                flat.Players[playerCount++] = ToBridge(player);
            }
        }

        std::size_t entityCount{ 0 };
        if (tick.Entities)
        {
            for (const Entity& entity : tick.Entities->Rows)
            {
                if (entityCount == flat.Entities.size())
                {
                    flags |= static_cast<std::uint32_t>(BridgeTickFlag::EntitiesTruncated);
                    break;
                }
                flat.Entities[entityCount++] = ToBridge(entity);
            }
        }

        flat.Match = Zeroed<BridgeMatch>();
        if (tick.Match)
        {
            flat.Match.GameTick = tick.Match->GameTick;
            flat.Match.Phase = static_cast<std::uint8_t>(tick.Match->Phase);
            flat.Match.RoundState = static_cast<std::uint8_t>(tick.Match->RoundState);
            flat.Match.RoundIndex = tick.Match->RoundIndex;
            flat.Match.RoundClockTicks = tick.Match->RoundClockTicks;
            flat.Match.RoundTicksRemaining = tick.Match->RoundTicksRemaining;
            flat.Match.RoundTimeLimitTicks = tick.Match->RoundTimeLimitTicks;
            flat.Match.RoundLimit = tick.Match->RoundLimit;
            flat.Match.ScoreToWin = tick.Match->ScoreToWin;
            flat.Match.RespawnTimeTicks = tick.Match->RespawnTimeTicks;
            flat.Match.SuddenDeathTicks = tick.Match->SuddenDeathTicks;
            flat.Match.GracePeriodTicks = tick.Match->GracePeriodTicks;
            flat.Match.LivesPerRound = tick.Match->LivesPerRound;
            flat.Match.TeamLivesPerRound = tick.Match->TeamLivesPerRound;
            flat.Match.RoundsToWin = tick.Match->RoundsToWin;

            for (std::size_t powerup{ 0 }; powerup < tick.Match->PowerupDurationTicks.size() && powerup < flat.Match.PowerupDurationTicks.size(); ++powerup)
            {
                flat.Match.PowerupDurationTicks[powerup] = tick.Match->PowerupDurationTicks[powerup];
            }

            for (std::size_t team{ 0 }; team < tick.Match->TeamScores.size() && team < flat.Match.TeamRoundScores.size(); ++team)
            {
                flat.Match.TeamRoundScores[team] = tick.Match->TeamScores[team].Round;
                flat.Match.TeamTotalScores[team] = tick.Match->TeamScores[team].Total;
            }

            for (const MatchScore& score : tick.Match->Scores)
            {
                const std::size_t index{ score.PlayerHandle & k_HandleIndexMask };
                if (index < flat.Match.PlayerScores.size()) flat.Match.PlayerScores[index] = score.Score;
            }
        }

        // Flags and stands. Like the entities, only the rows below the count are meaningful.
        std::size_t objectiveCount{ 0 };
        std::size_t standCount{ 0 };
        if (tick.ObjectiveDetails)
        {
            const auto& details{ *tick.ObjectiveDetails };
            objectiveCount = FlattenObjectives(details.Flags, flat.Objectives);
            standCount = FlattenObjectives(details.Stands, flat.Stands);
            if (objectiveCount < details.Flags.size() || standCount < details.Stands.size())
            {
                flags |= static_cast<std::uint32_t>(BridgeTickFlag::ObjectivesTruncated);
            }
        }

        // Weapons, projectiles, explosives and lifts, by handle. Like the entities, only the rows below the count are meaningful.
        std::size_t weaponCount{ 0 };
        if (tick.WeaponDetails)
        {
            weaponCount = FlattenByHandle(*tick.WeaponDetails, flat.Weapons);
            if (weaponCount < tick.WeaponDetails->size()) flags |= static_cast<std::uint32_t>(BridgeTickFlag::WeaponsTruncated);
        }

        std::size_t projectileCount{ 0 };
        if (tick.ProjectileDetails)
        {
            projectileCount = FlattenByHandle(*tick.ProjectileDetails, flat.Projectiles);
            if (projectileCount < tick.ProjectileDetails->size()) flags |= static_cast<std::uint32_t>(BridgeTickFlag::ProjectilesTruncated);
        }

        std::size_t explosiveCount{ 0 };
        if (tick.ExplosiveDetails)
        {
            explosiveCount = FlattenByHandle(*tick.ExplosiveDetails, flat.Explosives);
            if (explosiveCount < tick.ExplosiveDetails->size()) flags |= static_cast<std::uint32_t>(BridgeTickFlag::ExplosivesTruncated);
        }

        std::size_t liftCount{ 0 };
        if (tick.LiftDetails)
        {
            liftCount = FlattenByHandle(*tick.LiftDetails, flat.Lifts);
            if (liftCount < tick.LiftDetails->size()) flags |= static_cast<std::uint32_t>(BridgeTickFlag::LiftsTruncated);
        }

        // One View per row of Players, in the same order as the rows above. The rows past the players of the tick are zero, padding included.
        std::uint32_t viewMask{ 0 };
        for (std::size_t row{ 0 }; row < flat.Views.size(); ++row)
        {
            if (tick.Views && row < playerCount)
            {
                flat.Views[row] = ToBridge((*tick.Views)[row]);
                if ((*tick.Views)[row].Head.IsComputed) viewMask |= (1u << row);
            }
            else
            {
                flat.Views[row] = Zeroed<BridgeView>();
            }
        }

        // The newest events of the log: Python keeps its cursor by sequence and skips the ones it already has.
        std::size_t eventCount{ 0 };
        std::uint32_t lastEventSequence{ 0 };
        if (tick.Events)
        {
            const auto& rows{ tick.Events->Rows };
            const std::size_t first{ rows.size() > flat.Events.size() ? rows.size() - flat.Events.size() : 0 };
            for (std::size_t i{ first }; i < rows.size(); ++i)
            {
                flat.Events[eventCount++] = ToBridge(rows[i]);
            }
            lastEventSequence = tick.Events->LastSequence;
        }

        flat.Header.EventCount = static_cast<std::uint16_t>(eventCount);
        flat.Header.LastEventSequence = lastEventSequence;
        flat.Header.PlayerCount = static_cast<std::uint16_t>(playerCount);
        flat.Header.ViewCount = static_cast<std::uint16_t>(playerCount);
        flat.Header.ViewMask = viewMask;
        flat.Header.EntityCount = static_cast<std::uint16_t>(entityCount);
        flat.Header.ObjectiveCount = static_cast<std::uint16_t>(objectiveCount);
        flat.Header.StandCount = static_cast<std::uint16_t>(standCount);
        flat.Header.WeaponCount = static_cast<std::uint16_t>(weaponCount);
        flat.Header.ProjectileCount = static_cast<std::uint16_t>(projectileCount);
        flat.Header.ExplosiveCount = static_cast<std::uint16_t>(explosiveCount);
        flat.Header.LiftCount = static_cast<std::uint16_t>(liftCount);
        flat.Header.Generation = tick.Generation;
        flat.Header.PossessedPlayer = k_NoHandle;
        flat.Header.PossessedInput = BridgeAgentInput{};
        flat.Header.MapId = tick.MapId;
        flat.Header.Flags = flags;
    }
}