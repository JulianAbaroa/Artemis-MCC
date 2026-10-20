export module Platform.Memory.Type:Signature;

export namespace Platform::Memory::Type::Signature
{
    // Byte pattern of a game function. "??" matches any byte.
    struct Signature
    {
        const char* Name{};
        const char* Pattern{};
    };

    // Functions: Lifecycle
    inline constexpr Signature k_EngineInitialize
    {
        "EngineInitialize",
        "48 8B C4 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC C0 00 00 00 48 C7 45 B7 FE FF FF FF",
    };

    inline constexpr Signature k_DestroySubsystems
    {
        "DestroySubsystems",
        "40 56 57 41 57 48 83 EC 40 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 68 48 89 6C 24 70 33 F6",
    };

    // MCC-Win64-Shipping.exe: picks the module of a game and creates its engine with the CreateGameEngine export (A15).
    inline constexpr Signature k_LoadGameEngine
    {
        "LoadGameEngine",
        "48 89 5C 24 10 57 48 83 EC 30 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 44 24 28 48 8B D9 48 63 FA 48 8B 0D ?? ?? ?? ?? 48 85 C9 74 ?? 39 BB EC BF 02 00"
    };

    // MCC-Win64-Shipping.exe: updates the lifecycle manager of the online layer, once per frame on the MCC thread (D124).
    inline constexpr Signature k_LifecycleUpdate
    {
        "LifecycleUpdate",
        "48 8B C4 57 48 83 EC 70 48 C7 40 B8 FE FF FF FF 48 89 58 10 48 89 70 18 48 8B F9 48 8D B1 C8 01 00 00 48 8B CE E8 ?? ?? ?? ?? 8A 87 98 00 00 00 84 C0 74 ?? C6 87 98 00 00 00 00"
    };

    // MCC-Win64-Shipping.exe: StartCountdown, called by the START of the lobby (D124). Artemis only reads two calls from it: the
    // session getter and the check of a running countdown.
    inline constexpr Signature k_StartCountdown
    {
        "StartCountdown",
        "48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 20 40 8A F1 33 FF FF 15 ?? ?? ?? ?? 84 C0 74 ?? 8D 4F 01 E8 ?? ?? ?? ?? 48 8B C8 E8 ?? ?? ?? ?? 8D 4F"
    };

    // MCC-Win64-Shipping.exe: inside the StartGame state, finds the custom games lobby screen by name in the UI manager (D124).
    // note: Not a function start. The UI manager global, the screen name and the FindScreen function are read from its operands.
    inline constexpr Signature k_FindLobbyScreen
    {
        "FindLobbyScreen",
        "48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B C8 48 85 C0 75 ?? 48 8B 0D ?? ?? ?? ?? 48 8D 15 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B C8 48 85 C0 74 ?? 48 8B 01 FF 90 10 01 00 00"
    };

    // Functions: Map
    inline constexpr Signature k_BlamOpenMap
    {
        "BlamOpenMap",
        "48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 80 FD FF FF 48 81 EC 80 03 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 70 02 00 00 33 FF"
    };

    // Functions: Objects
    inline constexpr Signature k_TelemetryIdModifier
    {
        "TelemetryIdModifier",
        "8B 0D ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 48 8B 04 C8",
    };

    // Functions: Ticks
    inline constexpr Signature k_SimulationTicks
    {
        "SimulationTicks",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 70 20 41 57 48 83 EC 20 8B 05 ? ? ? ? 4C 8B F2 65 48 8B 04 25 58 00 00 00 8B F9 44 8B 05 ? ? ? ? 41 BF 48 00 00 00"
    };

    inline constexpr Signature k_ExecuteGameFrame
    {
        "ExecuteGameFrame",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8D A8 28 29 FF FF B8 C0 D7 00 00 E8 ?? ?? ?? ?? 48 2B E0 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 B8 D6 00 00"
    };

    // Applies the player actions of one tick: void(simulation update). ExecuteGameFrame calls it (A9_SplitScreen.txt sec. 12.4).
    inline constexpr Signature k_ApplyPlayerActions
    {
        "ApplyPlayerActions",
        "48 8B C4 55 41 54 41 55 41 56 41 57 48 8D A8 38 FF FF FF 48 81 EC A0 01 00 00 48 C7 44 24 78 FE FF FF FF 48 89 58 10 48 89 70 18 48 89 78 20 0F 29 70 C8 0F 29 78 B8 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 70 48 8B F9 48 89 4C 24 70 8B 15 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 4C 8B 34 D0 B8 30 01 00 00"
    };

    // Clears a player action to the empty action of the engine: void(action). Every action the engine builds starts with it (0x628b0)
    // (A17 sec. 2, D119).
    inline constexpr Signature k_ClearPlayerAction
    {
        "ClearPlayerAction",
        "48 89 5C 24 08 57 48 83 EC 20 48 8D 59 7B 33 D2 41 B8 A8 00 00 00 48 83 E3 FC 48 8B F9 E8 ?? ?? ?? ?? 41 83 C8 FF C6 47 04 FF 66 44 89 47 44"
    };

    // Place in 0x5df80 (player control of a local player) that loads the player control tag of the globals: mov rcx, [table]
    // (rel32 at +3), add rcx, 0x418, then call the tag address resolver (0x15918, rel32 at +0x30). Its field +0x48 is the hold
    // threshold of buttons in seconds (A17 sec. 8, D121).
    inline constexpr Signature k_PlayerControlTagLoad
    {
        "PlayerControlTagLoad",
        "48 8B 0D ?? ?? ?? ?? 48 63 FA 48 81 C1 18 04 00 00 F3 44 0F 11 64 24 50 F3 44 0F 11 4C 24 60 89 7C 24 4C 89 5C 24 54 4C 89 65 18 48 89 45 00 E8 ?? ?? ?? ??"
    };

    // Functions: High-level Data
    inline constexpr Signature k_BuildGameEvent
    {
        "BuildGameEvent",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 44 89 48 20 57 41 54 41 55 41 56 41 57 48 83 EC 40 4C 8B B4 24 90 00 00 00",
    };

    // Functions: Host
    // Constructor of the game manager in MCC-Win64-Shipping.exe: lea rax, [vtable], mov [game manager], rax, then it clears the
    // first members (xor eax, eax, mov [+0x8], rax, mov [+0x10], eax, and [+0x18], rax, and [+0x20], rax).
    inline constexpr Signature k_GameManager
    {
        "GameManager",
        "48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 33 C0 48 89 05 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 48 21 05 ?? ?? ?? ?? 48 21 05"
    };

    inline constexpr Signature k_CanAcceptAnyJoinRequest
    {
        "CanAcceptAnyJoinRequest",
        "40 53 48 83 EC 20 33 DB F6 81 00 49 00 00 01 74 4D 48 81 C1 80 48 00 00"
    };

    // Session state check: mov ecx, [state], lea eax, [rcx - 6], cmp eax, 3, ja.
    inline constexpr Signature k_SessionJoinCheck
    {
        "SessionJoinCheck",
        "8B 8B F8 0E 06 00 8D 41 FA 83 F8 03 77 56"
    };

    // Functions: Simulation
    // players_set_machines(mask, identifiers): replaces the machines of the engine (A9 sec. 13.2).
    inline constexpr Signature k_SetMachines
    {
        "SetMachines",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 41 54 41 55 41 56 41 57 48 8D 68 C8 48 81 EC 10 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 00 44 8B 05 ?? ?? ?? ?? 48 8B F2 65 48 8B 04 25 58 00 00 00"
    };

    // simulation_player_update_apply_added(update): creates the player of an "added" player update (A9 sec. 13.2).
    inline constexpr Signature k_ApplyPlayerAdded
    {
        "ApplyPlayerAdded",
        "48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC 00 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 27 4C 63 39 33 DB 65 48 8B 04 25 58 00 00 00"
    };

    // player_set_configuration(player, configuration): applies a "configuration" player update (name, appearance, team. A team change
    // kills the unit and starts the respawn timer) (A9 sec. 13.11).
    inline constexpr Signature k_ApplyPlayerConfiguration
    {
        "ApplyPlayerConfiguration",
        "48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 30 FF FF FF 48 81 EC D0 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 C0 00 00 00 65 48 8B 04 25 58 00 00 00 48 8B DA 44 8B E1 8B 0D ?? ?? ?? ??"
    };

    // First step of the interaction of a player (0x4ef34): the objects next to its unit (weapons, vehicles, equipment, objectives).
    // void(player, interaction). The engine runs both steps every frame for each local player only, through 0x4f280 (A9 M5d).
    inline constexpr Signature k_GatherNearbyInteractions
    {
        "GatherNearbyInteractions",
        "48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 FC FF FF 48 81 EC 90 04 00 00 0F 29 70 B8 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 70 03 00 00 44 8B 05 ?? ?? ?? ?? 4C 8B FA 65 48 8B 04 25 58 00 00 00 8B F9"
    };

    // Next weapon slot of a unit (0x48b618): int16(unit, from slot, direction). What a local client asks for when its player
    // switches weapon (A17 sec. 6, D120).
    inline constexpr Signature k_NextWeaponSlot
    {
        "NextWeaponSlot",
        "40 53 48 83 EC 20 44 8B 0D ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 41 BB 10 00 00 00 4A 8B 04 C8 44 0F B7 C9 4E 8B 1C 18 4F 8D 14 49 49 8B 43 50 4E 8B 4C D0 10 49 0F BF 99 8E 01 00 00"
    };

    // Next grenade type of a unit with grenades left (0x48b58c): int16(unit, from type, cycle) (A17 sec. 6, D120).
    inline constexpr Signature k_NextGrenadeType
    {
        "NextGrenadeType",
        "48 89 5C 24 08 44 8B 0D ?? ?? ?? ?? 33 DB 65 48 8B 04 25 58 00 00 00 45 0F B7 D8 41 BA 10 00 00 00 41 83 C8 FF 66 44 3B C2 0F B7 C9"
    };

    // Next zoom level of the weapon of a unit (0x485838): int16(unit, current level), -1 at the end of the cycle (A17 sec. 6, D120).
    inline constexpr Signature k_NextZoomLevel
    {
        "NextZoomLevel",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC 20 41 BF 01 00 00 00 0F B7 FA 41 8A F7 48 83 CB FF E8 ?? ?? ?? ?? 8B 0D ?? ?? ?? ?? 44 8B C0"
    };

    // Whether a unit can zoom now (0x4852a0): bool(unit) (A17 sec. 6, D120).
    inline constexpr Signature k_CanUnitZoom
    {
        "CanUnitZoom",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC 20 8B 15 ?? ?? ?? ?? 41 83 CF FF 65 48 8B 04 25 58 00 00 00 40 B7 01 41 BC 10 00 00 00 8B D9"
    };

    // Second step of the interaction of a player (0x4eb54): objects inside a cone of its weapon. Same arguments (A9 M5d).
    inline constexpr Signature k_GatherAimedInteractions
    {
        "GatherAimedInteractions",
        "48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 B8 C0 22 00 00 E8 ?? ?? ?? ?? 48 2B E0 0F 29 B4 24 B0 22 00 00 0F 29 BC 24 A0 22 00 00 44 0F 29 84 24 90 22 00 00 44 0F 29 8C 24 80 22 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 70 22 00 00 44 8B 05 ?? ?? ?? ?? 45 33 E4 65 48 8B 04 25 58 00 00 00 4C 8B FA"
    };

    // Functions: Input Injection
    inline constexpr Signature k_GetButtonState
    {
        "GetButtonState",
        "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 0F BF D1 83 EA 69 74 ?? 83 EA 01 74 ?? 83 EA 01 74 ?? 83 FA 01 74"
    };

}