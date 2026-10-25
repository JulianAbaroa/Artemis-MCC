export module Platform.Lifecycle.Type;

import std;

export namespace Platform::Lifecycle::Type
{
    using LifecycleCallback = std::function<void()>;

    // Callback that receives the full path of a cache file the engine opened.
    using MapFileCallback = std::function<void(const std::string& filePath)>;

    // Logic ticks per second of the engine at x1 (fixed step of 1/60 s, D105).
    inline constexpr float k_TicksPerSecond{ 60.0f };

    // Signature of the engine initialization function.
    using EngineInitializeFunction = auto(__fastcall*)() -> void;

    // Signature of the engine function that opens a map file.
    // param mapRelativePath: Null terminated path of the map, relative to the game folder.
    using OpenMapFunction = auto(__fastcall*)(std::uint64_t param1,
        std::uint64_t param2, std::uint64_t mapRelativePath, std::uint32_t* param4) -> void;

    // Signature of the engine destruction function.
    using DestroySubsystemsFunction = auto(__fastcall*)() -> void;

    // Signature of the MCC function that picks the module of a game and creates its engine with the CreateGameEngine export.
    // note: Stores the engine in a global of MCC. Every later call of MCC to the engine goes through that global (A15).
    using LoadGameEngineFunction = auto(__fastcall*)(void* gameLaunch, std::int32_t game) -> void;

    // Inside the LoadGameEngine signature, "mov rcx, [rip + disp32]" reads the engine global: disp32 at +0x22, next instruction at +0x26.
    inline constexpr std::size_t k_EngineGlobalDisplacementOffset{ 0x22 };
    inline constexpr std::size_t k_EngineGlobalNextInstructionOffset{ 0x26 };

    // Entries of the engine interface vtable (IGameEngine of the game module, A15 sec. 2.5).
    inline constexpr std::size_t k_EngineVTableSize{ 11 };
    inline constexpr std::size_t k_EngineDestroyIndex{ 0 };
    inline constexpr std::size_t k_EngineStartIndex{ 2 };
    inline constexpr std::size_t k_EngineUpdateStateIndex{ 3 };
    inline constexpr std::size_t k_EngineCommandIndex{ 9 };

    // State command that leaves the match, as the Leave of the pause menu (A15 sec. 2.11). It carries no payload.
    inline constexpr std::int32_t k_EngineLeaveState{ 0x02 };

    // Bytes of the match configuration MCC passes to the engine start. The engine copies it to its runtime configuration.
    inline constexpr std::size_t k_EngineConfigurationSize{ 0x2BF30 };

    // Offset of the id of the base map in the match configuration. It is an id of MCC, not the one of Reach (A15 sec. 2.10).
    inline constexpr std::size_t k_ConfigurationBaseMapOffset{ 0x10 };

    // Range of bytes of a match configuration.
    struct ConfigurationBlock
    {
        std::size_t Offset{};
        std::size_t Size{};
    };

    // Blocks a recorded configuration gives to the configuration of MCC at the engine start (D123, A15 sec. 5.2): the id of the base
    // map (+0x10 and +0x14), +0x40 (changes with the base map, not identified), the game variant with its content header and its
    // metadata, and the map variant. The rest (flags, match id, lobby players, pointer to the game module) stays the one of MCC.
    inline constexpr std::array<ConfigurationBlock, 4> k_TransplantBlocks{ {
        { .Offset = k_ConfigurationBaseMapOffset, .Size = 0x8 },
        { .Offset = 0x40, .Size = 0x4 },
        { .Offset = 0x310, .Size = 0x10000 - 0x310 },
        { .Offset = 0x1CF00, .Size = k_EngineConfigurationSize - 0x1CF00 },
    } };

    // Longest name of a recorded match configuration, in characters.
    inline constexpr std::size_t k_MaxMatchConfigNameLength{ 64 };

    // Match configuration recorded at an engine start, with the name it was saved under (D123).
    struct MatchConfig
    {
        std::string Name{};
        std::vector<std::byte> Bytes{};
    };

    // State command of the engine interface that carries a payload, and the bytes of that payload.
    struct EngineStatePayload
    {
        std::int32_t State{};
        std::size_t Size{};
    };

    // Payloads of the state commands. The states not listed carry none (A15 sec. 2.5).
    inline constexpr std::array<EngineStatePayload, 6> k_EngineStatePayloads{ {
        { .State = 0x10, .Size = 8 },
        { .State = 0x11, .Size = 8 },
        { .State = 0x12, .Size = 16 },
        { .State = 0x13, .Size = 8 },
        { .State = 0x18, .Size = 8 },
        { .State = 0x19, .Size = 64 },
    } };

    // Bytes of the largest state payload.
    inline constexpr std::size_t k_EngineStatePayloadMaxSize{ 64 };

    // States whose payload is the address of a string (A15 sec. 2.7).
    inline constexpr std::int32_t k_EngineFirstTextState{ 0x10 };
    inline constexpr std::int32_t k_EngineLastTextState{ 0x11 };

    // Signature of the engine destructor (entry 0).
    using EngineDestroyFunction = auto(__fastcall*)(void* engine) -> void;

    // Signature of the engine start (entry 2): copies the configuration and starts the game thread.
    using EngineStartFunction = auto(__fastcall*)(void* engine, void* systemInterface, const std::byte* configuration) -> void;

    // Signature of the state command of the engine (entry 3): queues the state and its payload for the game thread.
    using EngineUpdateStateFunction = auto(__fastcall*)(void* engine, std::int32_t state, const void* payload) -> void;

    // Signature of the text command of the engine (entry 9): queues a string for the game thread ("HS: " runs HaloScript).
    using EngineCommandFunction = auto(__fastcall*)(void* engine, const char* command) -> void;

    // Signature of the MCC function that updates the lifecycle manager of the online layer (UFG::eLifeCycleState), once per frame on
    // the MCC thread (D124, A15 sec. 2.14).
    using LifecycleUpdateFunction = auto(__fastcall*)(void* manager) -> void;

    // Offset of the current state in the lifecycle manager, and the PreGame state (the lobby and also the main menu).
    inline constexpr std::size_t k_LifecycleStateOffset{ 0x40 };
    inline constexpr std::int32_t k_PreGameLifecycleState{ 2 };

    // Signature of the MCC function that finds a screen of the UI manager by name. Returns null if that screen is not open.
    using FindScreenFunction = auto(__fastcall*)(void* uiManager, const char* name) -> void*;

    // Signature of the START of the custom games lobby screen (entry k_LobbyStartIndex of its vtable): validates the lobby, writes
    // the chosen map and variants to the session, fills the loading screen and starts the countdown (A15 sec. 2.16).
    using LobbyStartFunction = auto(__fastcall*)(void* screen) -> void;

    // Entry of the START in the vtable of the custom games lobby screen (+0x1B8).
    inline constexpr std::size_t k_LobbyStartIndex{ 0x1B8 / sizeof(void*) };

    // Inside the FindLobbyScreen signature: "mov rcx, [rip + disp32]" reads the UI manager global (disp32 at +0x3, next instruction
    // at +0x7), "lea rdx, [rip + disp32]" loads the name of the lobby screen (+0xA, +0xE) and "call rel32" calls FindScreen (+0xF,
    // +0x13).
    inline constexpr std::size_t k_UiManagerDisplacementOffset{ 0x3 };
    inline constexpr std::size_t k_UiManagerNextInstructionOffset{ 0x7 };
    inline constexpr std::size_t k_LobbyNameDisplacementOffset{ 0xA };
    inline constexpr std::size_t k_LobbyNameNextInstructionOffset{ 0xE };
    inline constexpr std::size_t k_FindScreenDisplacementOffset{ 0xF };
    inline constexpr std::size_t k_FindScreenNextInstructionOffset{ 0x13 };

    // Signature of the MCC function that returns its session object. MCC always asks for kind 1.
    using GetSessionFunction = auto(__fastcall*)(std::int32_t kind) -> void*;
    inline constexpr std::int32_t k_SessionKind{ 1 };

    // Signature of the MCC function that tells whether the countdown of the lobby runs (bit 7 of +0xE of the session properties).
    using IsCountdownActiveFunction = auto(__fastcall*)(void* session) -> bool;

    // Signature of the MCC check that the map variant of the lobby supports its game variant. The START of the lobby shows the
    // "incompatible variants" dialog when it fails. The registers are passed through as they come (bench, 2026-10-21).
    using VariantsCompatibleFunction = auto(__fastcall*)(std::uint64_t first, std::uint64_t second, std::uint64_t third,
        std::uint64_t fourth) -> bool;

    // Inside the StartCountdown signature: "call rel32" to the session getter (+0x22, next instruction at +0x26) and to the countdown
    // check (+0x2A, +0x2E).
    inline constexpr std::size_t k_GetSessionDisplacementOffset{ 0x22 };
    inline constexpr std::size_t k_GetSessionNextInstructionOffset{ 0x26 };
    inline constexpr std::size_t k_IsCountdownActiveDisplacementOffset{ 0x2A };
    inline constexpr std::size_t k_IsCountdownActiveNextInstructionOffset{ 0x2E };

    // State of the game engine as seen by Artemis.
    enum class Status
    {
        // No engine instance yet. Initial state and the state after a reboot.
        Waiting,
        // Engine initialized. Waiting for simulation ticks.
        Initialized,
        // Simulation ticks are being processed.
        Running,
        // Engine is being destroyed. Artemis work must stop.
        TearingDown,
        // Engine destroyed, or hooks found corrupted. Waiting for a reboot.
        Destroyed,
    };
}