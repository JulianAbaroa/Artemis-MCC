export module Platform.Memory.Type:Signature;

export namespace Platform::Memory::Type::Signature
{
    // Byte pattern of a game function. "??" matches any byte.
    struct Signature
    {
        const char* Name;
        const char* Pattern;
    };

    // Functions: Lifecycle
    inline constexpr Signature EngineInitialize
    {
        "EngineInitialize",
        "48 8B C4 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC C0 00 00 00 48 C7 45 B7 FE FF FF FF",
    };

    inline constexpr Signature DestroySubsystems
    {
        "DestroySubsystems",
        "40 56 57 41 57 48 83 EC 40 48 C7 44 24 20 FE FF FF FF 48 89 5C 24 68 48 89 6C 24 70 33 F6",
    };

    // Functions: Map
    inline constexpr Signature BlamOpenMap
    {
        "BlamOpenMap",
        "48 89 5C 24 08 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 80 FD FF FF 48 81 EC 80 03 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 70 02 00 00 33 FF"
    };

    // Functions: Objects
    inline constexpr Signature TelemetryIdModifier
    {
        "TelemetryIdModifier",
        "8B 0D ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 48 8B 04 C8",
    };

    // Functions: Ticks
    inline constexpr Signature SimulationTicks
    {
        "SimulationTicks",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 70 20 41 57 48 83 EC 20 8B 05 ? ? ? ? 4C 8B F2 65 48 8B 04 25 58 00 00 00 8B F9 44 8B 05 ? ? ? ? 41 BF 48 00 00 00"
    };

    inline constexpr Signature ExecuteGameFrame
    {
        "ExecuteGameFrame",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 60 20 55 41 56 41 57 48 8D A8 28 29 FF FF B8 C0 D7 00 00 E8 ?? ?? ?? ?? 48 2B E0 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 B8 D6 00 00"
    };

    // Applies the player actions of one tick: void(simulation update). ExecuteGameFrame calls it (A9_SplitScreen.txt sec. 12.4).
    inline constexpr Signature ApplyPlayerActions
    {
        "ApplyPlayerActions",
        "48 8B C4 55 41 54 41 55 41 56 41 57 48 8D A8 38 FF FF FF 48 81 EC A0 01 00 00 48 C7 44 24 78 FE FF FF FF 48 89 58 10 48 89 70 18 48 89 78 20 0F 29 70 C8 0F 29 78 B8 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 70 48 8B F9 48 89 4C 24 70 8B 15 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 4C 8B 34 D0 B8 30 01 00 00"
    };

    // Functions: High-level Data
    inline constexpr Signature BuildGameEvent
    {
        "BuildGameEvent",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 44 89 48 20 57 41 54 41 55 41 56 41 57 48 83 EC 40 4C 8B B4 24 90 00 00 00",
    };

    // Functions: Host
    // Constructor of the game manager in MCC-Win64-Shipping.exe: lea rax, [vtable], mov [game manager], rax, then it clears the
    // first members (xor eax, eax, mov [+0x8], rax, mov [+0x10], eax, and [+0x18], rax, and [+0x20], rax).
    inline constexpr Signature GameManager
    {
        "GameManager",
        "48 8D 05 ?? ?? ?? ?? 48 89 05 ?? ?? ?? ?? 33 C0 48 89 05 ?? ?? ?? ?? 89 05 ?? ?? ?? ?? 48 21 05 ?? ?? ?? ?? 48 21 05"
    };

    inline constexpr Signature CanAcceptAnyJoinRequest
    {
        "CanAcceptAnyJoinRequest",
        "40 53 48 83 EC 20 33 DB F6 81 00 49 00 00 01 74 4D 48 81 C1 80 48 00 00"
    };

    // Session state check: mov ecx, [state], lea eax, [rcx - 6], cmp eax, 3, ja.
    inline constexpr Signature SessionJoinCheck
    {
        "SessionJoinCheck",
        "8B 8B F8 0E 06 00 8D 41 FA 83 F8 03 77 56"
    };

    // Functions: Simulation
    // players_set_machines(mask, identifiers): replaces the machines of the engine (A9 sec. 13.2).
    inline constexpr Signature SetMachines
    {
        "SetMachines",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 55 41 54 41 55 41 56 41 57 48 8D 68 C8 48 81 EC 10 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 00 44 8B 05 ?? ?? ?? ?? 48 8B F2 65 48 8B 04 25 58 00 00 00"
    };

    // simulation_player_update_apply_added(update): creates the player of an "added" player update (A9 sec. 13.2).
    inline constexpr Signature ApplyPlayerAdded
    {
        "ApplyPlayerAdded",
        "48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC 00 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 45 27 4C 63 39 33 DB 65 48 8B 04 25 58 00 00 00"
    };

    // player_set_configuration(player, configuration): applies a "configuration" player update (name, appearance, team; a team change
    // kills the unit and starts the respawn timer) (A9 sec. 13.11).
    inline constexpr Signature ApplyPlayerConfiguration
    {
        "ApplyPlayerConfiguration",
        "48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 30 FF FF FF 48 81 EC D0 01 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 C0 00 00 00 65 48 8B 04 25 58 00 00 00 48 8B DA 44 8B E1 8B 0D ?? ?? ?? ??"
    };

    // First step of the interaction of a player (0x4ef34): the objects next to its unit (weapons, vehicles, equipment, objectives).
    // void(player, interaction). The engine runs both steps every frame for each local player only, through 0x4f280 (A9 M5d).
    inline constexpr Signature GatherNearbyInteractions
    {
        "GatherNearbyInteractions",
        "48 8B C4 48 89 58 18 55 56 57 41 54 41 55 41 56 41 57 48 8D A8 38 FC FF FF 48 81 EC 90 04 00 00 0F 29 70 B8 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 70 03 00 00 44 8B 05 ?? ?? ?? ?? 4C 8B FA 65 48 8B 04 25 58 00 00 00 8B F9"
    };

    // Second step of the interaction of a player (0x4eb54): objects inside a cone of its weapon. Same arguments (A9 M5d).
    inline constexpr Signature GatherAimedInteractions
    {
        "GatherAimedInteractions",
        "48 89 5C 24 18 55 56 57 41 54 41 55 41 56 41 57 B8 C0 22 00 00 E8 ?? ?? ?? ?? 48 2B E0 0F 29 B4 24 B0 22 00 00 0F 29 BC 24 A0 22 00 00 44 0F 29 84 24 90 22 00 00 44 0F 29 8C 24 80 22 00 00 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 84 24 70 22 00 00 44 8B 05 ?? ?? ?? ?? 45 33 E4 65 48 8B 04 25 58 00 00 00 4C 8B FA"
    };

    // Functions: Input Injection
    inline constexpr Signature GetButtonState
    {
        "GetButtonState",
        "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 0F BF D1 83 EA 69 74 ?? 83 EA 01 74 ?? 83 EA 01 74 ?? 83 FA 01 74"
    };

}