#pragma once

#include <cstdint>
#include <string>

enum class ObjectRole : uint8_t
{
    Unknown = 0,
    None,

    // --- bipd ---

    SelfBiped,
    Biped,

    // --- vehi ---

    Vehicle,
    VehiclePart,

    // --- weap ---

    WeaponPickup,
    WeaponEquiped,

    ObjectivePickup,
    ObjectiveEquiped,

    // --- eqip ---

    // Armor Ability.
    ArmorAbilityPickup,
    ArmorAbilityEquiped,

    // Grenade.
    GrenadePickup,

    // Ammo.
    AmmoPickup,

    // --- proj ---

    Projectile,

    // --- scen ---

    // Spawn.
    Spawn,

    // Boundaries.
    Boundary,

    // Portable Shield.
    PortableShield,

    // Obstacle.
    SceneryObstacle,

    // --- mach ---

    DeviceMachine,

    // --- bloc ---

    // Objective.
    ObjectiveSpawn,

    // Teleporter.
    Teleporter,

    // Explosive.
    Explosive,

    // Pallet.
    Pallet,

    // Lift.
    Lift,

    // Shield.
    Shield,

    // Obstacle.
    CrateObstacle,

    // --- ctrl ---

    HealthStation,
};

struct ClassifiedObject
{
    uint32_t Handle;
    ObjectRole Role;
};