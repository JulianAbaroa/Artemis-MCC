export module Platform.Lifecycle.Hook:EngineInterface;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Memory.System;
import Platform.Lifecycle.Type;
import std;

namespace
{
    using Platform::Lifecycle::Type::k_EngineVTableSize;
    using Platform::Lifecycle::Type::k_EngineStatePayloadMaxSize;
}

export namespace Platform::Lifecycle::Hook
{
    // Logs every call of MCC to the engine interface: creation, start (with a dump of the match configuration), state commands, text
    // commands and destruction (A15).
    // note: Hooks the MCC function that creates the engine, which stays loaded for the whole process. Each new engine gets a copy of its
    // vtable with the logged entries replaced, so nothing is patched inside the game module. Development tool: it only logs.
    class EngineInterfaceDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using AOBService = Platform::Memory::System::AOBService;
        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using LoadGameEngineFunction = Platform::Lifecycle::Type::LoadGameEngineFunction;
        using EngineDestroyFunction = Platform::Lifecycle::Type::EngineDestroyFunction;
        using EngineStartFunction = Platform::Lifecycle::Type::EngineStartFunction;
        using EngineUpdateStateFunction = Platform::Lifecycle::Type::EngineUpdateStateFunction;
        using EngineCommandFunction = Platform::Lifecycle::Type::EngineCommandFunction;

    public:
        EngineInterfaceDetour(LogsService& logsService, SettingsStore& settingsStore, AOBService& aobService,
            MemoryReaderService& memoryReaderService) :
            m_LogsService(logsService), m_SettingsStore(settingsStore), m_AOBService(aobService),
            m_MemoryReaderService(memoryReaderService) {}
        ~EngineInterfaceDetour() = default;

        // Finds the MCC function by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Disables the hook, gives the live engine its original vtable back, waits for the calls in flight, then removes the hook.
        // note: Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        SettingsStore& m_SettingsStore;
        AOBService& m_AOBService;
        MemoryReaderService& m_MemoryReaderService;

        static inline EngineInterfaceDetour* s_Instance{ nullptr };
        static inline LoadGameEngineFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook or a replaced vtable entry. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        // vtable of the game module that the traced engine had, used to call the original entries.
        static inline void** s_OriginalVTable{ nullptr };

        // Copy of the vtable the traced engine points to, with the logged entries replaced.
        static inline std::array<void*, k_EngineVTableSize> s_TracedVTable{};

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Global of MCC that holds the current engine.
        void** m_EngineGlobal{ nullptr };

        // Engine that points to the traced vtable, or null. Guarded by m_TraceMutex.
        void* m_TracedEngine{ nullptr };

        // Serializes tracing, untracing and the destruction of the traced engine (MCC threads and the main thread).
        std::mutex m_TraceMutex{};

        // Engine starts seen by this process. Numbers the configuration dumps.
        std::atomic<std::uint32_t> m_StartCount{ 0 };

        // Last state command logged and how many times it repeated since. Equal consecutive commands are folded into a count.
        std::mutex m_StateMutex{};
        std::int32_t m_LastState{ -1 };
        std::array<std::byte, k_EngineStatePayloadMaxSize> m_LastPayload{};
        std::uint32_t m_RepeatCount{ 0 };

        static auto __fastcall HookedLoadGameEngine(void* gameLaunch, std::int32_t game) -> void;
        static auto __fastcall HookedEngineDestroy(void* engine) -> void;
        static auto __fastcall HookedEngineStart(void* engine, void* systemInterface, const std::byte* configuration) -> void;
        static auto __fastcall HookedEngineUpdateState(void* engine, std::int32_t state, const void* payload) -> void;
        static auto __fastcall HookedEngineCommand(void* engine, const char* command) -> void;

        // Points the engine to a copy of its vtable with the logged entries replaced.
        // note: MCC keeps one engine at a time, so a previous engine still marked as traced was freed without the interface.
        auto Trace(void* engine, std::int32_t game) -> void;

        // Gives the traced engine its original vtable back, if it is still the engine of MCC and points to the copy.
        auto Untrace() -> void;

        // Writes the match configuration to a file in the storage folder.
        auto DumpConfiguration(const std::byte* configuration) -> void;

        // Logs a state command, or counts it if it equals the previous one.
        auto LogState(std::int32_t state, const void* payload) -> void;

        // Logs how many times the previous state command repeated, if it did.
        // note: The caller holds m_StateMutex.
        auto FlushRepeats() -> void;

        // return: The string at the address, empty if it cannot be read.
        auto ReadText(std::uintptr_t address) -> std::string;
    };
}