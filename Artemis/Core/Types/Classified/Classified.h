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

    ArmorAbilityPickup,
    ArmorAbilityEquiped,
    GrenadePickup,
    AmmoPickup,

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