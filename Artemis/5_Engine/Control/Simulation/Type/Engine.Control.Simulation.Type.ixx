export module Engine.Control.Simulation.Type;

import Common.Engine.Type;
import Control.Action.Type;
import std;

// Layout of the engine simulation players and machines (A9_SplitScreen.txt sec. 13).
namespace
{
    using ::Control::Action::Type::UnitClientState;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_GamerTagLength;
}

export namespace Engine::Control::Simulation::Type
{
    // Machines the engine tracks (16 players and the host).
    inline constexpr std::size_t k_MaxMachines{ 17 };

    // Players one machine can have: the user index and the controller (0 to 3) cannot repeat inside a machine.
    inline constexpr std::size_t k_MaxPlayersPerMachine{ 4 };

    // Bytes of a machine identifier.
    inline constexpr std::size_t k_MachineIdentifierSize{ 6 };

    // Slots of the thread context: objects data array, players data array, players globals and game options.
    inline constexpr std::uintptr_t k_ObjectsSlot{ 0x10 };
    inline constexpr std::uintptr_t k_PlayersSlot{ 0x18 };
    inline constexpr std::uintptr_t k_PlayersGlobalsSlot{ 0x118 };
    inline constexpr std::uintptr_t k_GameOptionsSlot{ 0x48 };

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
    inline constexpr std::uintptr_t k_PlayerUnit{ 0x28 };
    inline constexpr std::uintptr_t k_PlayerGamerTag{ 0xB0 };

    // Team of the player datum (int8) and its copy of the configuration (k_PlayerConfigurationSize bytes, starts with the gamertag).
    inline constexpr std::uintptr_t k_PlayerTeam{ 0xAD };
    inline constexpr std::uintptr_t k_PlayerConfiguration{ 0xB0 };

    // Object header (0x18 bytes per object in the objects data array): pointer to the object data at +0x10.
    inline constexpr std::size_t k_ObjectHeaderSize{ 0x18 };
    inline constexpr std::uintptr_t k_ObjectHeaderData{ 0x10 };

    // Unit flags (uint32). 0x47050 applies the action of a player only if bit 0 of its unit is set.
    inline constexpr std::uintptr_t k_UnitFlags{ 0x1C4 };

    // Action slot of a unit (uint8). 0x47050 moves the unit with the action (0x4d7d0, unit_control) only if the action has +0x00 = 1
    // and +0x04 equal to this byte. The engine builds that for local players. A bot needs it written (A9 sec. 13.10).
    inline constexpr std::uintptr_t k_UnitActionSlot{ 0x288 };

    // Client state of a unit, the fields a client reads to ask for a weapon, a grenade or a zoom level (A17 sec. 2 and 6, D120):
    // weapon slot in the hands (int8, -1 none), version of the inventory (uint16, (+1) % 7 on every change: a weapon request is
    // only taken if the action carries this value), grenade type (int8) and zoom level (int8, -1 not zoomed).
    inline constexpr std::uintptr_t k_UnitWeaponSlot{ 0x34A };
    inline constexpr std::uintptr_t k_UnitInventoryVersion{ 0x34C };
    inline constexpr std::uintptr_t k_UnitGrenadeType{ 0x37D };
    inline constexpr std::uintptr_t k_UnitZoomLevel{ 0x384 };

    // Parent object of an object (uint32 handle, 0xFFFFFFFF none) and seat of a unit inside its parent (int16, -1 none). A unit with
    // both sits in a vehicle or a turret.
    inline constexpr std::uintptr_t k_ObjectParent{ 0x14 };
    inline constexpr std::uintptr_t k_UnitSeat{ 0x336 };

    // Desired aim of a unit: 3 floats, the direction its aim turns toward (unit datum, measured).
    inline constexpr std::uintptr_t k_UnitDesiredAim{ 0x208 };

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

    // Team inside a player configuration (int8). Team games clamp it to 0..7, other games use the player index.
    inline constexpr std::uintptr_t k_ConfigurationTeam{ 0x78 };

    // Teams of a team game (0 red, 1 blue, 2 green, 3 orange, 4 purple, 5 gold, 6 brown, 7 pink).
    inline constexpr std::size_t k_MaxTeams{ 8 };

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

    // Interaction of a player as the engine builds it (A9 M5d): records of the objects the player can use, best first. A record has
    // the kind (uint16) at +0x00 and the object handle (uint32) at k_InteractionRecordObject. An empty record is zeros with the handle
    // at 0xFFFFFFFF.
    inline constexpr std::size_t k_InteractionRecordCount{ 3 };
    inline constexpr std::size_t k_InteractionRecordSize{ 0x0C };
    inline constexpr std::uintptr_t k_InteractionRecordObject{ 0x08 };

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
        std::int8_t Team{ -1 };
        std::uint32_t UnitHandle{ 0xFFFFFFFF };
        std::uint32_t UnitFlags{ 0 };
        std::array<wchar_t, k_GamerTagLength> GamerTag{};

        // Client state of the unit (D120). Empty without a live unit.
        std::optional<UnitClientState> Client{};
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

    // Team change asked for a bot, applied on the next tick.
    struct TeamRequest
    {
        std::size_t PlayerIndex{ 0 };
        std::int8_t Team{ 0 };
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

    // Signature of player_set_configuration: applies a configuration to the player (handle with salt).
    using ApplyPlayerConfigurationFunction = auto(__fastcall*)(std::uint32_t player, const void* configuration) -> void;

    // Signature of the engine selectors of the next weapon slot (0x48b618) and the next grenade type (0x48b58c) of a unit.
    // return: The slot or type, or -1 if there is none.
    using UnitSelectorFunction = auto(__fastcall*)(std::uint32_t unit, std::int16_t from, std::int16_t step) -> std::int16_t;

    // Signature of the engine selector of the next zoom level of the weapon of a unit (0x485838).
    // return: The level, or -1 at the end of the cycle (not zoomed).
    using NextZoomLevelFunction = auto(__fastcall*)(std::uint32_t unit, std::int16_t level) -> std::int16_t;

    // Signature of the engine check of whether a unit can zoom now (0x4852a0).
    using CanUnitZoomFunction = auto(__fastcall*)(std::uint32_t unit) -> bool;

    // Signature of the two steps that fill the interaction of a player (handle with salt) once it was cleared (0x4ef34, 0x4eb54).
    using GatherInteractionsFunction = auto(__fastcall*)(std::uint32_t player, void* interaction) -> void;
}