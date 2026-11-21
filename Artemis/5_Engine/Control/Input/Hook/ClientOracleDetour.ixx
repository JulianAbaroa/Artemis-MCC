export module Engine.Control.Input.Hook:ClientOracle;

import Common.Log.System;
import Control.Client.Type;
import Engine.Base.Memory.System;
import Engine.Control.Input.Type;
import Engine.Control.Input.State;
import Engine.Control.Input.System;
import Engine.Control.Simulation.System;
import std;

export namespace Engine::Control::Input::Hook
{
    // Oracle of the pure client (VW.9c.1, D356): hooks the player control of the engine (0x5df80) and, for a local human player without
    // an override, runs the pure client on the table, the block and the unit it saw at the entry, then compares its output and its state
    // with what the engine left. Development tool, off by default (the shadow check switch of PlayerActionStore).
    // note: Reads game memory and never writes it. Runs on the game thread inside the original call.
    class ClientOracleDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Engine::Base::Memory::System::AOBService;

        using ClientSettings = ::Control::Client::Type::ClientSettings;
        using ClientStep = ::Control::Client::Type::ClientStep;
        using UnitContext = ::Control::Client::Type::UnitContext;

        using ClientDifferences = Engine::Control::Input::Type::ClientDifferences;
        using ControlTagLocator = Engine::Control::Input::Type::ControlTagLocator;
        using PlayerControlFunction = Engine::Control::Input::Type::PlayerControlFunction;

        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;


        using BotService = Engine::Control::Simulation::System::BotService;

    public:
        ClientOracleDetour(Log& log, AOBService& aobService, PlayerActionStore& playerActionStore, BotService& botService) :
            m_Log(log), m_AOBService(aobService), m_PlayerActionStore(playerActionStore), m_BotService(botService) {}
        ~ClientOracleDetour() = default;

        // Finds the player control by its rva and the control tag anchor, and installs the hook.
        // return: False if the function is not found (the log says why) or the hook fails. The oracle stays off and nothing else changes.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, removes the hook and writes what the buffer still holds. Does nothing if not installed.
        auto Uninstall() -> void;

        // Writes the lines the hook buffered to the oracle file, and logs the lines it had to drop.
        // note: Main thread only (it does file I/O, which the game thread never does). The Main loop calls it every second.
        auto Flush() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        PlayerActionStore& m_PlayerActionStore;
        BotService& m_BotService;

        static inline std::atomic<ClientOracleDetour*> s_Instance{ nullptr };
        static inline PlayerControlFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Base of haloreach.dll, derived from the function address and its rva.
        std::uintptr_t m_ModuleBase{ 0 };

        // Where the control tag is, read from the anchor found inside the hooked function.
        ControlTagLocator m_ControlTag{};

        // Variable that holds the thread local storage index, or null if not found.
        const std::uint32_t* m_EngineTlsIndex{ nullptr };

        // Path of the oracle file, resolved once at Install. Empty if LOCALAPPDATA is not set (then nothing is written).
        std::string m_OraclePath{};

        // Context switches the log reports once: the table is not in the mouse state and a call read facts the context does not know.
        // Game thread only.
        bool m_IsWrongModeLogged{ false };
        bool m_IsPartialContextLogged{ false };

        // Lines of the oracle file waiting for the Main thread, at most k_ClientBufferLines. A full buffer drops the line and counts it.
        // The game thread appends (formatting only, never I/O) and Flush takes them. Guarded by m_BufferMutex.
        std::mutex m_BufferMutex{};
        std::deque<std::string> m_BufferedLines{};
        std::uint64_t m_DroppedLines{ 0 };

        // Lines accepted for the file since Install (they count against k_ClientMaxFileLines whether written yet or not). Guarded by
        // m_BufferMutex.
        std::uint64_t m_AcceptedLines{ 0 };

        // Lines accepted per kind of difference (k_ClientLinesPerKind). Guarded by m_BufferMutex.
        std::unordered_map<std::string, std::uint32_t> m_LinesPerKind{};

        static auto __fastcall HookedPlayerControl(std::int32_t inputUser, std::uint32_t localIndex, float deltaTime, float unused,
            void* tableOut, float* output) -> void;

        // Compares one call. Game thread only.
        // param capture: What the entry saw (the table, the client block and the output), as raw bytes.
        auto Check(std::int32_t inputUser, std::uint32_t localIndex, float deltaTime, std::uintptr_t threadContext, std::size_t playerIndex,
            std::span<const std::uint8_t> table, std::span<const std::uint8_t> blockBefore, std::span<const std::uint8_t> blockAfter,
            std::span<const std::uint8_t> outputBefore, std::span<const std::uint8_t> outputAfter) -> void;

        // return: The settings of the local user the client reads, or nullopt when the control tag cannot be read.
        auto ReadSettings(std::uint32_t localIndex, std::uintptr_t threadContext) const -> std::optional<ClientSettings>;

        // return: The dual wield restrictions byte of the local user: the keyboard and mouse global, or the byte of its pad settings.
        // nullopt without the module base (the DualWield fact then stays unknown).
        auto ReadDualWieldFlags(std::uint32_t localIndex) const -> std::optional<std::uint8_t>;

        // Adds the counts of the call to the store and logs the first mismatches. A mismatch also queues its line for the file.
        auto Report(const ClientStep& step, const ClientDifferences& differences, const UnitContext& context, bool isZoomed,
            std::int32_t inputUser, float deltaTime, std::span<const std::uint8_t> table, std::span<const std::uint8_t> blockBefore) -> void;

        // Queues one line for the file, or drops it (and counts it) when the file is full or the buffer is.
        auto Enqueue(std::string line) -> void;
    };
}