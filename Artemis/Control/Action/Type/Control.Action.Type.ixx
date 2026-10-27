export module Control.Action.Type;

import Layout.Action.Type;
import std;

namespace
{
    using Layout::Action::Type::k_HoldLanes;
    using Layout::Action::Type::k_NoZoomLevel;
}

export namespace Control::Action::Type
{
    // Fields Artemis writes into the action of a player, every tick (A9 M4b, D109).
    // note: The action of a player with an override starts from the empty action of the engine (D119): the unit slot, the interaction
    // (D118) and these fields go on top. The rest (weapon, grenade and zoom requests, aim assist) stays empty for locals and bots.
    struct ActionOverride
    {
        std::uint64_t Buttons{ 0 };

        // Desired facing, absolute, in radians.
        float Yaw{ 0.0f };
        float Pitch{ 0.0f };

        // Most radians the yaw and the pitch the hook writes move per tick toward Yaw and Pitch (D114). 0 = straight to them.
        float TurnLimit{ 0.0f };

        float ThrottleForward{ 0.0f };
        float ThrottleLeft{ 0.0f };

        // Radians added to Yaw after every tick. Test only (the AI will write the yaw of each tick).
        float YawPerTick{ 0.0f };

        // Down bits of the hold lanes the agent holds this tick (k_HoldLanes). The held bits are added by the hook (D121).
        std::uint32_t Lanes{ 0 };

        // Holds use, then releases it, in a cycle. Test only (A9 M5d: can a bot pick up the flag).
        bool IsUseTest{ false };

        // Local player (split-screen input user) of the player. Its control block gets the same facing, so its view follows the
        // action. Empty for a player that is not local.
        std::optional<std::size_t> LocalIndex{};
    };

    // Yaw and pitch the hook last wrote for an agent, and the unit they were written for (D114).
    struct AppliedFacing
    {
        std::uint32_t Unit{ 0 };
        std::array<float, 2> Facing{};
    };

    // One-tick presses of an agent that the client turns into requests (D120): the hook consumes them on its next tick.
    struct ClientRequests
    {
        // Switch to the next weapon (a tap of switch weapon).
        bool IsWeaponSwitch{ false };

        // Switch to the next grenade type with grenades left.
        bool IsGrenadeSwitch{ false };

        // Go to the next zoom level of the weapon in the hands, or back to no zoom after the last one.
        bool IsZoom{ false };
    };

    // Weapon slot an agent asked for, sent until the unit holds it or the ticks run out (the engine drops a request whose inventory
    // version is stale).
    // note: The request is dropped when the inventory version of the unit changes (a pickup or a drop by the host): its slot may no
    // longer hold the same weapon.
    struct WeaponRequest
    {
        std::int8_t Slot{ -1 };
        std::uint16_t InventoryVersion{ 0 };
        std::uint32_t TicksLeft{ 0 };
    };

    // Client state of the unit of a player on one tick (A17 sec. 6).
    struct UnitClientState
    {
        std::uint32_t Unit{ 0xFFFFFFFF };
        std::int8_t WeaponSlot{ -1 };
        std::uint16_t InventoryVersion{ 0 };
        std::int8_t GrenadeType{ -1 };
        std::int8_t ZoomLevel{ k_NoZoomLevel };

        // The unit sits in a vehicle or a turret.
        bool IsSeated{ false };

        // Yaw in [0, 2 pi) and pitch of the desired aim of the unit, in radians.
        std::array<float, 2> Facing{};
    };

    // What the action of an agent remembers between ticks (D213). Its owner is the detour, one per player.
    // note: Forgotten when the agent has no override, so a new override starts from the unit again.
    struct PlayerActionHistory
    {
        // Facing the agent got on its last tick (D114). Empty without a unit.
        std::optional<AppliedFacing> Facing{};

        // Ticks each hold lane has been down (D121).
        std::array<std::uint32_t, k_HoldLanes.size()> LaneTicks{};

        // Weapon slot the agent asked for and the unit does not hold yet (D120).
        std::optional<WeaponRequest> PendingWeapon{};
    };
}