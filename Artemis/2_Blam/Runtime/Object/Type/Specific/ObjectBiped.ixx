export module Blam.Runtime.Object.Type:Biped;

import Common.Math.Type;
import Blam.MapModel.Definitions.Type;
import std;

namespace
{
    using Common::Math::Type::Vec2;
    using Common::Math::Type::Vec3;
    using Blam::MapModel::Definitions::Type::Object::DamageReportingType;
}

export namespace Blam::Runtime::Object::Type::Biped
{
    // Zoom stage of the biped.
    enum class ZoomLevel : std::uint8_t
    {
        None = 0xFF,
        Zoom1 = 0x00,
        Zoom2 = 0x01,
    };

    // Jump and crouch keys held by the controlling player.
    // note: The engine only registers a key after it stays held for several ticks. A quick tap is never reflected.
    enum class VerticalInput : std::uint8_t
    {
        None = 0,
        Crouch = 1,
        Jump = 2,
        JumpCrouch = 3,
    };

    // Type of the selected grenade. The same indices hold for spartans and elites.
    enum class GrenadeType : std::uint8_t
    {
        Frag = 0,
        Plasma = 1,
    };

    // Last hit of one attacker (unit 0x4DC, one slot of 0x18 bytes per attacker).
    struct Damage
    {
        // Game tick of the last hit, the clock of TicksAlive and TicksSinceDamage.
        std::uint32_t Tick{};

        // Health and shield damage of the last hit, as fractions of their maximum.
        // note: Not accumulated. Several hits of the same attacker keep only the values of the last one.
        float Health{};
        float Shield{};

        // Biped and player that dealt it. The victim itself on its own grenade or a fall.
        std::uint32_t BipedHandle{};
        std::uint32_t PlayerHandle{};

        // Weapon or cause of the last hit.
        // note: A fall that slid down a slope reported Collision, not Falling.
        DamageReportingType ReportingType{};
    };

    struct Biped
    {
        // Ticks since the unit started updating, usually since it spawned.
        std::uint32_t TicksAlive{};

        // Aim direction of the unit, smoothed by the engine towards the desired aim.
        Vec3 Aim{};

        // Movement keys held by the controlling player. It is an input, not a world direction.
        // note: X is forward (+1 W, -1 S) and Y is strafe (+1 A, -1 D). It does not depend on the camera. Reads zero when no key is held.
        // note: Only keyboard was observed. Analog values from a gamepad are untested.
        Vec2 MovementInput{};

        VerticalInput VerticalInput{};

        // Armor ability key state. Its meaning depends on the ability. The Equipment of ArmorAbilityHandle has the real state.
        // note: Sprint and camo stay 1 for as long as the ability runs, even with the key released.
        // note: Jetpack stays 1 only while the key is held. Armor lock is assumed to behave the same, but it is untested.
        // note: Drop shield, hologram and evade follow the key, not the effect. A canceled drop shield animation returns to 0.
        // note: Hologram can spawn without ever reading 1, and holding the evade key does not trigger more evades.
        std::uint8_t IsAbilityActive{};

        // Confirmed reliable in every case.
        ZoomLevel ZoomLevel{};

        // Unit normal of the ground surface the biped last touched.
        // note: Valid on static geometry and on living objects. It is not cleared in the air, so it is the last contact.
        // note: Observed z of 1.0 on flat floor and 0.69 on steep slopes.
        Vec3 SurfaceNormal{};

        // Handle of the living object the biped stands on.
        // note: 0xFFFFFFFF on static geometry and in the air. It lags the real contact by a fraction of a second.
        std::uint32_t GroundObjectHandle{};

        // Weapons held, by slot. They do not swap when the biped switches weapons.
        std::uint32_t PrimaryWeaponHandle{};
        std::uint32_t SecondaryWeaponHandle{};

        // Weapon in the hands of the biped.
        // note: 0xFFFFFFFF in a vehicle or without a usable weapon.
        std::uint32_t EquippedWeaponHandle{};

        // Equipment object of the armor ability. Its state is in the Equipment of that handle.
        std::uint32_t ArmorAbilityHandle{};

        // note: Kept when the grenades run out. Picking up the other type while empty selects it, so it is always a type the biped holds, if any.
        GrenadeType SelectedGrenade{};
        std::uint8_t FragGrenades{};
        std::uint8_t PlasmaGrenades{};

        // Attackers since the shield last started recharging, at most four, in order of their first hit.
        // note: Empty once the shield starts recharging. Kept after death, so the killer is the one with the latest Tick.
        std::vector<Damage> Damages{};

        // Game tick of the last damage, or 0xFFFFFFFF if the biped was never damaged. Kept when Damages empties.
        std::uint32_t LastDamageTick{};

        // Ticks since the last damage, or nothing if the biped was never damaged.
        std::optional<std::uint32_t> TicksSinceDamage{};
    };
}