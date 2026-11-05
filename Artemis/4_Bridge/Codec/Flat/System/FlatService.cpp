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
        flat.Header.EntityCount = static_cast<std::uint16_t>(entityCount);
        flat.Header.ObjectiveCount = static_cast<std::uint16_t>(objectiveCount);
        flat.Header.StandCount = static_cast<std::uint16_t>(standCount);
        flat.Header.WeaponCount = static_cast<std::uint16_t>(weaponCount);
        flat.Header.ProjectileCount = static_cast<std::uint16_t>(projectileCount);
        flat.Header.ExplosiveCount = static_cast<std::uint16_t>(explosiveCount);
        flat.Header.LiftCount = static_cast<std::uint16_t>(liftCount);
        flat.Header.Generation = tick.Generation;
        flat.Header.MapId = tick.MapId;
        flat.Header.Flags = flags;
    }
}