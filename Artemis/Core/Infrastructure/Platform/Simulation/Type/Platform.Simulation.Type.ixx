export module Platform.Simulation.Type;

import std;

// Layout of the engine simulation players and machines (A9_SplitScreen.txt sec. 13).
export namespace Platform::Simulation::Type
{
    // Machines the engine tracks (16 players and the host).
    inline constexpr std::size_t k_MaxMachines{ 17 };

    // Players the engine tracks.
    inline constexpr std::size_t k_MaxPlayers{ 16 };

    // Players one machine can have: the user index and the controller (0 to 3) cannot repeat inside a machine.
    inline constexpr std::size_t k_MaxPlayersPerMachine{ 4 };

    // Bytes of a machine identifier.
    inline constexpr std::size_t k_MachineIdentifierSize{ 6 };

    // Slots of the thread context: players data array, players globals and game options.
    inline constexpr std::uintptr_t k_PlayersSlot{ 0x18 };
    inline constexpr std::uintptr_t k_PlayersGlobalsSlot{ 0x118 };
    inline constexpr std::uintptr_t k_GameOptionsSlot{ 0x48 };

    // Data array: maximum count (int32), element size (int32) and first element (pointer).
    // note: +0x44 is the first slot never used, not the capacity: free slots above it are still valid (datum_new_at_index).
    inline constexpr std::uintptr_t k_DataArrayCapacity{ 0x2C };
    inline constexpr std::uintptr_t k_DataArrayElementSize{ 0x20 };
    inline constexpr std::uintptr_t k_DataArrayData{ 0x50 };

    // Player datum: salt (uint16, 0 = free slot), identifier (uint64), machine index (int16), user index (int32), controller (int32)
    // and gamertag (16 wchar_t).
    inline constexpr std::uintptr_t k_PlayerSalt{ 0x00 };

    // Player flags (uint32): bit 0 active in game (set by the activation events of the simulation queue), bit 3 set on creation in
    // network games, bit 15 joined in progress.
    inline constexpr std::uintptr_t k_PlayerFlags{ 0x04 };
    inline constexpr std::uintptr_t k_PlayerIdentifier{ 0x08 };

    // Machine identifier of the player datum (6 bytes). The engine keeps it next to the machine index, and players_update_activation
    // (0x46e6c) reads it to find the simulation view of the machine (A9 sec. 13.8).
    inline constexpr std::uintptr_t k_PlayerMachineIdentifier{ 0x14 };
    inline constexpr std::uintptr_t k_PlayerMachineIndex{ 0x1A };
    inline constexpr std::uintptr_t k_PlayerUserIndex{ 0x1C };
    inline constexpr std::uintptr_t k_PlayerController{ 0x20 };
    inline constexpr std::uintptr_t k_PlayerGamerTag{ 0xB0 };

    // Characters of a gamertag, terminator included.
    inline constexpr std::size_t k_GamerTagLength{ 16 };

    // Players globals: machine mask (uint32), machine identifiers (17 x 6 bytes) and local machine index (int32).
    inline constexpr std::uintptr_t k_MachineMask{ 0x04 };
    inline constexpr std::uintptr_t k_MachineIdentifiers{ 0x08 };
    inline constexpr std::uintptr_t k_LocalMachineIndex{ 0x78 };

    // Game options: one entry per player (by player index) with the configuration at +0x20.
    inline constexpr std::uintptr_t k_OptionsPlayers{ 0x2D8 };
    inline constexpr std::size_t k_OptionsPlayerSize{ 0xA0 };
    inline constexpr std::uintptr_t k_OptionsPlayerConfiguration{ 0x20 };

    // Bytes of a player configuration (name, team, appearance).
    inline constexpr std::size_t k_PlayerConfigurationSize{ 0x80 };

    // Player update of the simulation (simulation_player_update).
    inline constexpr std::size_t k_PlayerUpdateSize{ 0xB8 };
    inline constexpr std::uintptr_t k_UpdatePlayerIndex{ 0x00 };
    inline constexpr std::uintptr_t k_UpdateIdentifier{ 0x04 };
    inline constexpr std::uintptr_t k_UpdateType{ 0x0C };
    inline constexpr std::uintptr_t k_UpdateMachine{ 0x10 };
    inline constexpr std::uintptr_t k_UpdateUserIndex{ 0x18 };
    inline constexpr std::uintptr_t k_UpdateController{ 0x1C };
    inline constexpr std::uintptr_t k_UpdateJoinedInProgress{ 0x21 };
    inline constexpr std::uintptr_t k_UpdateConfiguration{ 0x28 };

    // Type of a player update that adds a player.
    inline constexpr std::int32_t k_PlayerUpdateAdded{ 3 };

    // Identifier of a machine, as the engine stores it.
    struct MachineIdentifier
    {
        std::array<std::uint8_t, k_MachineIdentifierSize> Bytes{};
    };

    static_assert(sizeof(MachineIdentifier) == k_MachineIdentifierSize);

    // One player of the engine, as the session section shows it.
    struct SessionPlayer
    {
        bool IsValid{ false };
        std::uint32_t Handle{ 0 };
        std::uint32_t Flags{ 0 };
        std::uint64_t Identifier{ 0 };
        std::int16_t MachineIndex{ -1 };
        std::int32_t UserIndex{ -1 };
        std::int32_t Controller{ -1 };
        std::array<wchar_t, k_GamerTagLength> GamerTag{};
    };

    // Machines and players of the engine on one tick, plus the game options entry of every player (A9 M5a).
    struct SessionSnapshot
    {
        std::uint64_t Sequence{ 0 };
        std::uint32_t MachineMask{ 0 };
        std::array<MachineIdentifier, k_MaxMachines> Machines{};
        std::int32_t LocalMachineIndex{ -1 };
        std::array<SessionPlayer, k_MaxPlayers> Players{};
        std::array<std::array<std::uint8_t, k_OptionsPlayerSize>, k_MaxPlayers> Options{};
    };

    // A player Artemis added on a machine of its own (A9 M5b).
    struct Bot
    {
        // Number of the bot, from 1, in the order they were added.
        std::size_t Number{ 0 };
        std::size_t PlayerIndex{ 0 };
        std::size_t MachineIndex{ 0 };
        // User index and controller inside its machine.
        std::size_t Slot{ 0 };
        std::uint64_t Identifier{ 0 };
    };

    // A machine that only exists for the bots.
    struct FakeMachine
    {
        std::size_t Index{ 0 };
        MachineIdentifier Identifier{};
    };

    // Signature of players_set_machines: replaces the machines of the engine and drops the players of the machines that are gone.
    using SetMachinesFunction = auto(__fastcall*)(std::uint32_t mask, const MachineIdentifier* identifiers) -> void;

    // Signature of simulation_player_update_apply_added: creates the player of an "added" update.
    using ApplyPlayerAddedFunction = auto(__fastcall*)(const void* update) -> bool;
}