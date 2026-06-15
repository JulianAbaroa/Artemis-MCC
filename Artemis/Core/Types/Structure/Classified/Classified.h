#pragma once

#include <cstdint>
#include <string>

enum class ObjectRole : uint8_t
{
    Unknown = 0,
    None,

    // --- bipd ---

    Biped,

    // --- vehi ---

    Vehicle,
    VehiclePart,

    // --- weap ---

    WeaponPickup,
    WeaponEquipped,

    ObjectivePickup,
    ObjectiveEquipped,

    // --- eqip ---

    ArmorAbilityPickup,
    ArmorAbilityEquipped,
    GrenadePickup,
    AmmoPickup,
    Powerup,

    // --- proj ---

    Projectile,

    // --- scen ---

    Spawn,
    Boundary,
    PortableShield,
    SceneryObstacle,

    // --- mach ---

    DeviceMachine,

    // --- bloc ---

    ObjectiveSpawn,
    ObjectiveZone,
    Teleporter,
    Explosive,
    Pallet,
    Lift,
    Shield,
    CrateObstacle,

    // --- ctrl ---

    HealthStation,
};

struct Classified
{
    uint32_t Handle;
    ObjectRole Role;
};