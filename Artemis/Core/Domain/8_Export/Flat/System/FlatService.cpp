module Export.Flat.System;

import Common.Engine.Type;
import Common.Math.Type;
import Environment.Entities.Type;
import Environment.Players.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
    using Environment::Entities::Type::Entity;
    using Environment::Players::Type::Player;
    using Platform::Bridge::Type::BridgeVec3;
    using Platform::Bridge::Type::BridgePlayer;
    using Platform::Bridge::Type::BridgeEntity;
    using Platform::Bridge::Type::BridgeFrameFlag;

    using Common::Engine::Type::k_NoHandle;
    using Platform::Bridge::Type::k_BridgeNotLocal;

    // Value of an optional float that is absent (Shield, Vitality).
    constexpr float k_Absent{ std::numeric_limits<float>::quiet_NaN() };

    auto ToBridge(const Vec3& v) -> BridgeVec3
    {
        return BridgeVec3{ .X = v.X, .Y = v.Y, .Z = v.Z };
    }

    auto ToBridge(const Player& player) -> BridgePlayer
    {
        BridgePlayer row{};
        row.Handle = player.Handle;
        row.BipedHandle = player.BipedHandle;
        row.Team = static_cast<std::uint8_t>(player.Team);
        row.LocalIndex = player.LocalIndex.value_or(k_BridgeNotLocal);
        row.Connection = static_cast<std::uint8_t>(player.Connection);
        row.IsAlive = player.IsAlive() ? 1 : 0;
        row.CameraPosition = ToBridge(player.CameraPosition);
        row.CameraForward = ToBridge(player.CameraForward);
        row.TicksSinceDamage = k_NoHandle;

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
        return row;
    }

    auto ToBridge(const Entity& entity) -> BridgeEntity
    {
        BridgeEntity row{};
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
}

namespace Export::Flat::System
{
    auto FlatService::Flatten(const Tick& tick, BridgeFrame& frame) const -> void
    {
        std::uint32_t flags{ 0 };

        std::size_t playerCount{ 0 };
        if (tick.Players)
        {
            for (const Player& player : tick.Players->Rows)
            {
                if (playerCount == frame.Players.size()) break;
                frame.Players[playerCount++] = ToBridge(player);
            }
        }

        std::size_t entityCount{ 0 };
        if (tick.Entities)
        {
            for (const Entity& entity : tick.Entities->Rows)
            {
                if (entityCount == frame.Entities.size())
                {
                    flags |= static_cast<std::uint32_t>(BridgeFrameFlag::EntitiesTruncated);
                    break;
                }
                frame.Entities[entityCount++] = ToBridge(entity);
            }
        }

        frame.Header.PlayerCount = static_cast<std::uint16_t>(playerCount);
        frame.Header.EntityCount = static_cast<std::uint16_t>(entityCount);
        frame.Header.Generation = tick.Generation;
        frame.Header.Flags = flags;
    }
}