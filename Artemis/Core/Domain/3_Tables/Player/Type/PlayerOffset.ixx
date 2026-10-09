export module Tables.Player.Type:Offset;

import std;

export namespace Tables::Player::Type::Offset
{
    // Salt of the player handle (uint32).
    // note: The handle is the salt in the high 16 bits and the table index in the low 16 bits. The networking systems generate the salt.
    constexpr std::uintptr_t k_Handle{ 0x000 };

    // Connection state (uint8). 0x01 is Connected, 0x02 is Disconnected and 0x08 is Connecting.
    constexpr std::uintptr_t k_ConnectionState{ 0x004 };

    // Xuid of the player (uint64).
    constexpr std::uintptr_t k_Xuid{ 0x008 };

    // Network ID of the player (uint32).
    constexpr std::uintptr_t k_NetworkID{ 0x014 };

    // Handle of the biped the player controls (uint32).
    constexpr std::uintptr_t k_AliveBipedHandle{ 0x028 };

    // Handle of the last biped the player controlled (uint32).
    // note: Kept while the player is dead.
    constexpr std::uintptr_t k_DeadBipedHandle{ 0x02C };

    // Handle of the biped the player is using (uint32).
    // note: Changes when the game assigns another biped to the player.
    constexpr std::uintptr_t k_CurrentBipedHandle{ 0x034 };

    // Position of the camera (3 floats).
    constexpr std::uintptr_t k_CameraPosition{ 0x038 };

    // Forward vector of the camera (3 floats).
    constexpr std::uintptr_t k_CameraForward{ 0x044 };

    // Related to the aim, not fully understood (3 floats).
    constexpr std::uintptr_t k_AimOffset{ 0x050 };

    // Handle of the primary weapon (uint32).
    constexpr std::uintptr_t k_PrimaryWeaponHandle{ 0x05C };

    // Handle of the secondary weapon (uint32).
    constexpr std::uintptr_t k_SecondaryWeaponHandle{ 0x060 };

    // Handle of the objective while the player carries it (uint32).
    constexpr std::uintptr_t k_ObjectiveHandle{ 0x064 };

    // Second copy of the camera position (3 floats).
    constexpr std::uintptr_t k_CameraPosition2{ 0x08C };

    // Team of the player (uint8).
    constexpr std::uintptr_t k_Team{ 0x0AD };

    // Gamertag as 16 wide characters (32 bytes).
    constexpr std::uintptr_t k_GamerTag{ 0x0B0 };

    // Service tag as 4 wide characters (8 bytes).
    constexpr std::uintptr_t k_Tag{ 0x0F4 };
}