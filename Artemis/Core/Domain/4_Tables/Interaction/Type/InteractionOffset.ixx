export module Tables.Interaction.Type:Offset;

import std;

export namespace Tables::Interaction::Type::Offset
{
    // Kind of the active interaction (uint8). GrabWeapon, EnterVehicle, etcetera.
    constexpr std::uintptr_t k_Kind{ 0x00 };

    // Detail of the interaction (uint8).
    // note: Depends on the kind. A seat for vehicles, or grab and change for weapons.
    constexpr std::uintptr_t k_Detail{ 0x04 };

    // Handle of the object selected to interact with (uint32).
    constexpr std::uintptr_t k_TargetObjectHandle{ 0x08 };

    // Melee flag (uint8). It is 0x0E when a melee hit is available.
    // note: Does not cover the extended range of the sword.
    constexpr std::uintptr_t k_IsMeleeAvailable{ 0x0C };

    // Handle of the biped selected to melee (uint32).
    constexpr std::uintptr_t k_MeleeTargetHandle{ 0x14 };

    // Aim assist flag (uint8). It is 0x01 when a biped is close to or on the crosshair.
    constexpr std::uintptr_t k_IsAimAvailable{ 0x24 };

    // Part of the body of the aim target (uint8). 0x00 is the chest and 0x01 is the head.
    constexpr std::uintptr_t k_BipedBodyPart{ 0x28 };

    // Handle of the aimed biped (uint32).
    constexpr std::uintptr_t k_AimTargetHandle{ 0x2C };

    // Slot ID of the aimed player (uint32).
    constexpr std::uintptr_t k_AimTargetSlotID{ 0x30 };

    // How far the crosshair is from the center of the target (3 floats).
    // note: Higher values are farther from the center.
    constexpr std::uintptr_t k_AimHitLocalPosition{ 0x3C };
}