export module Tables.Player.Type:Offset;

import std;

export namespace Tables::Player::Type::Offset
{
	// Player table handle, composed from a Salt and a Index.
	// The salt is generated via networking systems (2 byte value)
	// The index is just the position inside the array of the player table.
	constexpr std::uintptr_t Handle{ 0x000 };			// uint32.
	
	// 0x01: Connected, 0x02: Disconnected, 0x08: Connecting.
	constexpr std::uintptr_t ConnectionState{ 0x004 };	// uint8.
	
	constexpr std::uintptr_t Xuid{ 0x008 };				// uint64.
	constexpr std::uintptr_t NetworkID{ 0x014 };			// uint32.
	
	// Holds the biped's handle that's being controlled by the player.
	constexpr std::uintptr_t AliveBipedHandle{ 0x028 };	// uint32.
	
	// Holds the last biped's handle that was controlled 
	// by the player, while the player is dead.
	constexpr std::uintptr_t DeadBipedHandle{ 0x02C };	// uint32.
	
	// Holds the biped's handle that the player is using.
	// It changes when the game assings another biped to the player.
	constexpr std::uintptr_t CurrentBipedHandle{ 0x034 };	// uint32.
	
	constexpr std::uintptr_t WeaponPosition{ 0x038 };		// 3 floats.
	constexpr std::uintptr_t WeaponForward{ 0x044 };		// 3 floats.
	
	// Related to aim, this needs more study.
	constexpr std::uintptr_t AimOffset{ 0x050 };			// 3 floats.
	
	constexpr std::uintptr_t PrimaryWeaponHandle{ 0x05C };	// uint32.
	constexpr std::uintptr_t SecondaryWeaponHandle{ 0x060 };	// uint32.
	
	// Holds the handle of the objective while the player is carrying it.
	constexpr std::uintptr_t ObjectiveHandle{ 0x064 };		// uint32.
	
	constexpr std::uintptr_t CameraPosition{ 0x08C };			// 3 floats.
	
	constexpr std::uintptr_t Team{ 0xAD };	// uint8.
	
	constexpr std::uintptr_t GamerTag{ 0xB0 };	// 32 bytes.
	constexpr std::uintptr_t Tag{ 0xF4 };		// 8 bytes.
}