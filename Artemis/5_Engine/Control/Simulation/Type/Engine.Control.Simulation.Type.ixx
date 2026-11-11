export module Engine.Control.Simulation.Type;

import Common.Engine.Type;
import Control.Action.Type;
import std;

// Layout of the engine simulation players and machines (A9_SplitScreen.txt sec. 13).
namespace
{
    using ::Control::Action::Type::UnitClientState;

    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_MaxTeams;
    using Common::Engine::Type::k_GamerTagLength;
}

export namespace Engine::Control::Simulation::Type
{
    // Machines the engine tracks (16 players and the host).
    inline constexpr std::size_t k_MaxMachines{ 17 };

    // Players one machine can have: the user index and the controller (0 to 3) cannot repeat inside a machine.
    inline constexpr std::size_t k_MaxPlayersPerMachine{ 4 };

    // Bots one instance can hold: 4 per fake machine, on every machine slot except the first one (the local machine). The player table
    // (k_MaxPlayers) is the limit that binds first.
    inline constexpr std::size_t k_MaxBots{ (k_MaxMachines - 1) * k_MaxPlayersPerMachine };

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

    // Player flags (uint32): bit 0 active in game (set by the activation events of the simulation queue), bit 1 left (the "left" update
    // sets it, the "remove" update needs it), bit 3 set on creation in network games, bit 15 joined in progress.
    inline constexpr std::uintptr_t k_PlayerFlags{ 0x04 };
    inline constexpr std::uint32_t k_PlayerFlagLeft{ 0x2 };
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

    // Types of a player update. Left needs the flag k_PlayerFlagLeft off and sets it, remove needs it on and frees the slot.
    inline constexpr std::int32_t k_PlayerUpdateLeft{ 0 };
    inline constexpr std::int32_t k_PlayerUpdateRemove{ 2 };
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
        // Number of the bot, from 1: the lowest one free when it was added. It decides its machine and user.
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

    // A bot as the roster planner sees it: its number, its slot in the player table and the team the engine has it on.
    struct RosterBot
    {
        std::size_t Number{ 0 };
        std::size_t PlayerIndex{ 0 };
        std::int8_t Team{ 0 };
    };

    // The bots a match should have (D284): how many on each team, and how many more on the team of the local player 0, which is
    // resolved when the roster is applied. The local players are not part of it.
    struct Roster
    {
        std::array<std::size_t, k_MaxTeams> Teams{};
        std::size_t Auto{ 0 };

        auto operator==(const Roster&) const -> bool = default;
    };

    // A bot that changes team.
    struct RosterMove
    {
        std::size_t Number{ 0 };
        std::int8_t Team{ 0 };

        auto operator==(const RosterMove&) const -> bool = default;
    };

    // What it takes to go from the bots of the match to a roster. It is applied in this order: moves, removes, adds.
    struct RosterPlan
    {
        std::vector<RosterMove> Moves{};
        // Numbers of the bots to remove.
        std::vector<std::size_t> Removes{};
        // Team of every bot to add.
        std::vector<std::int8_t> Adds{};

        auto IsEmpty() const -> bool
        {
            return Moves.empty() && Removes.empty() && Adds.empty();
        }

        auto operator==(const RosterPlan&) const -> bool = default;
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

    // Signature of simulation_players_apply_update (0x418ff8): applies a player update of the types left, swap, remove, added and
    // configuration. For left, remove and configuration it checks that the player of the index has the identifier of the update.
    // return: True if it applied the update.
    using ApplyPlayerUpdateFunction = auto(__fastcall*)(const void* update) -> bool;

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

    // return: A player update of the given type with the player index and the identifier, and every other field zero.
    // note: Enough for the types left and remove. The type added needs the machine, the user and the configuration on top.
    auto MakePlayerUpdate(std::int32_t playerIndex, std::uint64_t identifier, std::int32_t type) -> std::array<std::uint8_t, k_PlayerUpdateSize>
    {
        std::array<std::uint8_t, k_PlayerUpdateSize> update{};

        std::memcpy(update.data() + k_UpdatePlayerIndex, &playerIndex, sizeof(playerIndex));
        std::memcpy(update.data() + k_UpdateIdentifier, &identifier, sizeof(identifier));
        std::memcpy(update.data() + k_UpdateType, &type, sizeof(type));

        return update;
    }

    // return: The lowest bot number from 1 to k_MaxBots that is not in use, or nothing if all are. A bot that is removed and added again
    // gets the same number, so it gets the same machine, user and controller.
    auto FindFreeBotNumber(std::span<const std::size_t> used) -> std::optional<std::size_t>
    {
        for (std::size_t number{ 1 }; number <= k_MaxBots; ++number)
        {
            if (std::ranges::find(used, number) == used.end()) return number;
        }

        return std::nullopt;
    }

    // Plans the fewest operations that turn the bots of the match into the roster (D284). Per team it keeps the bots it has up to the
    // count it wants. The extra bots of a team move to the teams that miss some, in team order, and the ones left are removed. The
    // teams that still miss bots get new ones. Inside a team the bot with the highest number goes first, so the last added one leaves.
    // param bots: The bots of the match. A bot on a team that is not 0 to k_MaxTeams - 1 counts as an extra one.
    // param localTeam: Team of the local player 0, where the auto bots go.
    // param otherPlayers: Players in the table that are not bots (the local players and real ones).
    // return: The plan, or the reason the roster does not fit: more bots than k_MaxBots, or than the free slots of the player table.
    auto PlanRoster(std::span<const RosterBot> bots, const Roster& roster, std::int8_t localTeam, std::size_t otherPlayers)
        -> std::expected<RosterPlan, std::string>
    {
        std::array<std::size_t, k_MaxTeams> wanted{ roster.Teams };

        if (roster.Auto > 0)
        {
            if (localTeam < 0 || static_cast<std::size_t>(localTeam) >= k_MaxTeams)
            {
                return std::unexpected(std::format("the local player is on team {}, not on a team from 0 to {}", localTeam,
                    k_MaxTeams - 1));
            }

            wanted[static_cast<std::size_t>(localTeam)] += roster.Auto;
        }

        const std::size_t total{ std::accumulate(wanted.begin(), wanted.end(), std::size_t{ 0 }) };
        if (total > k_MaxBots) return std::unexpected(std::format("{} bots, at most {}", total, k_MaxBots));

        if (otherPlayers + total > k_MaxPlayers)
        {
            return std::unexpected(std::format("{} bots and {} other players do not fit in {} player slots", total, otherPlayers,
                k_MaxPlayers));
        }

        // Teams in order, the ones outside the range last, and the highest number first inside a team.
        std::vector<RosterBot> sorted{ bots.begin(), bots.end() };
        const auto teamOrder = [](const RosterBot& bot) {
            return (bot.Team >= 0 && static_cast<std::size_t>(bot.Team) < k_MaxTeams) ? static_cast<std::size_t>(bot.Team) : k_MaxTeams;
        };
        std::ranges::sort(sorted, [&teamOrder](const RosterBot& left, const RosterBot& right) {
            if (teamOrder(left) != teamOrder(right)) return teamOrder(left) < teamOrder(right);
            return left.Number > right.Number;
        });

        std::array<std::size_t, k_MaxTeams + 1> have{};
        for (const RosterBot& bot : sorted) ++have[teamOrder(bot)];

        // The first bots of each team in this order are its extra ones.
        std::vector<std::size_t> extra{};
        std::array<std::size_t, k_MaxTeams + 1> seen{};
        for (const RosterBot& bot : sorted)
        {
            const std::size_t team{ teamOrder(bot) };
            const std::size_t keep{ team < k_MaxTeams ? (std::min)(have[team], wanted[team]) : 0 };

            if (seen[team]++ < have[team] - keep) extra.push_back(bot.Number);
        }

        RosterPlan plan{};
        std::size_t next{ 0 };

        for (std::size_t team{ 0 }; team < k_MaxTeams; ++team)
        {
            for (std::size_t missing{ have[team] < wanted[team] ? wanted[team] - have[team] : 0 }; missing > 0; --missing)
            {
                if (next < extra.size()) plan.Moves.push_back(RosterMove{ .Number = extra[next++], .Team = static_cast<std::int8_t>(team) });
                else                     plan.Adds.push_back(static_cast<std::int8_t>(team));
            }
        }

        plan.Removes.assign(extra.begin() + static_cast<std::ptrdiff_t>(next), extra.end());
        return plan;
    }
}