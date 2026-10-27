export module Layout.Action.Type:Offset;

import std;

export namespace Layout::Action::Type::Offset
{
    // Mask of the players with an action (uint32, one bit per player table index), inside the simulation update.
    inline constexpr std::uintptr_t k_UpdateActionMaskOffset{ 0x08 };

    // First action inside the simulation update. The action of a player is at this offset + k_PlayerActionSize * its table index.
    inline constexpr std::uintptr_t k_UpdateActionsOffset{ 0x10 };

    // Unit of the action: +0x00 (uint32) = 1 and +0x04 (uint8) = action slot of the unit (unit +0x288), so the engine moves that
    // unit with the action (A9 sec. 13.10). The empty action has 0 and 0xFF. The engine writes them for a local player (0x628b0).
    inline constexpr std::uintptr_t k_ActionHasUnitOffset{ 0x00 };
    inline constexpr std::uintptr_t k_ActionUnitSlotOffset{ 0x04 };

    // Buttons of the action (uint64). Bits measured in A9 sec. 12.7 (crouch 0, jump 1, melee 11, zoom 18, fire 30, grenade 40...).
    inline constexpr std::uintptr_t k_ActionButtonsOffset{ 0x08 };

    // Desired yaw, pitch and roll of the action (3 floats, absolute, radians). The roll is always 0.
    // note: The engine keeps the yaw in [0, 2 pi).
    inline constexpr std::uintptr_t k_ActionFacingOffset{ 0x10 };

    // Throttle of the action (2 floats, -1 to 1): forward, then left.
    inline constexpr std::uintptr_t k_ActionThrottleOffset{ 0x1C };

    // Hold lanes of the action (uint32): a "down" bit per button and a "held" bit once it was down for the hold threshold of the
    // engine (A17 sec. 8, D121). The host takes the presses, holds and releases from them (0x4b534).
    inline constexpr std::uintptr_t k_ActionLanesOffset{ 0x40 };

    // Client requests of the action (A17 sec. 3, D120): echo of the inventory version of the unit (uint16), weapon slot (uint8),
    // grenade type (uint16) and zoom level (uint16). The empty action asks for nothing: 0xFFFF, 0xFF, 0xFFFF and 0xFFFF.
    // note: A weapon request only counts if the echo matches unit +0x34C.
    inline constexpr std::uintptr_t k_ActionInventoryVersionOffset{ 0x44 };
    inline constexpr std::uintptr_t k_ActionWeaponSlotOffset{ 0x46 };
    inline constexpr std::uintptr_t k_ActionGrenadeTypeOffset{ 0x48 };
    inline constexpr std::uintptr_t k_ActionZoomLevelOffset{ 0x4A };

    // Interaction of the action: the objects the engine picked for the player, best first (A9 sec. 12.2, M5d).
    // note: The engine fills it every frame for local players only, from their unit (0x4f280). The PlayerAction hook fills it every
    // tick for every player with an override, with the same engine functions (D118).
    inline constexpr std::uintptr_t k_ActionInteractionOffset{ 0x54 };
}