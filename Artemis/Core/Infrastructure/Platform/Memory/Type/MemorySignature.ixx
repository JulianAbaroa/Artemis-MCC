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

    inline constexpr Signature CreateObject
    {
        "CreateObject",
        "40 53 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 8B 41 ?? 49 83 CC"
    };

    inline constexpr Signature ReleaseObject
    {
        "ReleaseObjectByHandle",
        "48 89 5C 24 08 57 48 83 EC 20 8B 15 ?? ?? ?? ?? 65 48 8B 04 25 58 00 00 00 8B D9"
    };

    // Functions: Bone matrix
    inline constexpr Signature InitRootNode
    {
        "InitRootNode",
        "48 89 5C 24 08 48 89 74 24 10 57 48 81 EC 90 00 00 00 33 D2 48 8B D9 E8 ?? ?? ?? ?? 4C 8B 41 38 33 FF 48 63 D0 83 CE FF 3B D6"
    };

    // Functions: Players
    inline constexpr Signature CreatePlayer
    {
        "CreatePlayer",
        "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 44 8B 0D ?? ?? ?? ?? 48 8B FA 65 48 8B 04 25 58 00 00 00 8B D1 41 8A E8 4A 8B 34 C8 B8 18 00 00 00 48 8B 0C 30 E8 ?? ?? ?? ?? 8B D8 83 F8 FF 74"
    };

    // Functions: Ticks
    inline constexpr Signature SimulationTicks
    {
        "SimulationTicks",
        "48 8B C4 48 89 58 08 48 89 70 10 48 89 78 18 4C 89 70 20 41 57 48 83 EC 20 8B 05 ? ? ? ? 4C 8B F2 65 48 8B 04 25 58 00 00 00 8B F9 44 8B 05 ? ? ? ? 41 BF 48 00 00 00"
    };

    // Functions: High-level Data
    inline constexpr Signature BuildGameEvent
    {
        "BuildGameEvent",
        "48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 44 89 48 20 57 41 54 41 55 41 56 41 57 48 83 EC 40 4C 8B B4 24 90 00 00 00",
    };

    // Functions: Input Injection
    inline constexpr Signature GetButtonState
    {
        "GetButtonState",
        "?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 0F BF D1 83 EA 69 74 ?? 83 EA 01 74 ?? 83 EA 01 74 ?? 83 FA 01 74"
    };

}