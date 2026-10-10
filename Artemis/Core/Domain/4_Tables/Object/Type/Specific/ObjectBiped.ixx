export module Tables.Object.Type:Biped;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec2;
    using Common::Math::Type::Vec3;
}

export namespace Tables::Object::Type::Biped
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

    struct Biped
    {
        // Movement keys held by the controlling player. It is an input, not a world direction.
        // note: X is forward (+1 W, -1 S) and Y is strafe (+1 A, -1 D). It does not depend on the camera. Reads zero when no key is held.
        // note: Only keyboard was observed. Analog values from a gamepad are untested.
        Vec2 MovementInput{};

        VerticalInput VerticalInput{};

        // Armor ability state. Its meaning depends on the ability.
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

        // Handle of the biped that last damaged this one.
        // note: 0xFFFFFFFF when nobody did. Reset to 0xFFFFFFFF once the shield starts recharging. Kept after death, so it identifies the killer.
        std::uint32_t DamagerBipedHandle{};

        // Handle of the player that last damaged this one. It follows the same lifetime as DamagerBipedHandle.
        std::uint32_t DamagerPlayerHandle{};
    };
}