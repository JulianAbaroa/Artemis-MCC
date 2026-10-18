export module Platform.Input.Hook:PlayerAction;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Input.Type;
import Platform.Input.State;
import Platform.Simulation.System;
import std;

export namespace Platform::Input::Hook
{
    // Hooks the engine function that applies the player actions of each tick: writes the overrides into them, then publishes them (D109).
    // note: Players without an override keep the action the engine built. Called by ExecuteGameFrame once per tick on the game thread.
    // Before the actions, the bot service adds the bots asked for and publishes the session snapshot (A9 M5).
    class PlayerActionDetour
    {
    private:
        using LogsService = Service::Logs::System::LogsService;

        using AOBService = Platform::Memory::System::AOBService;

        using ApplyPlayerActionsFunction = Platform::Input::Type::ApplyPlayerActionsFunction;

        using PlayerActionStore = Platform::Input::State::PlayerActionStore;

        using BotService = Platform::Simulation::System::BotService;

    public:
        PlayerActionDetour(LogsService& logsService, AOBService& aobService, PlayerActionStore& playerActionStore,
            BotService& botService) :
            m_LogsService(logsService), m_AOBService(aobService), m_PlayerActionStore(playerActionStore), m_BotService(botService) {}
        ~PlayerActionDetour() = default;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        LogsService& m_LogsService;
        AOBService& m_AOBService;
        PlayerActionStore& m_PlayerActionStore;
        BotService& m_BotService;

        static inline PlayerActionDetour* s_Instance{ nullptr };
        static inline ApplyPlayerActionsFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Ticks seen since the hook was installed. Game thread only.
        std::uint64_t m_Sequence{ 0 };

        // Variable of the engine that holds the thread local storage index of its thread context, or null if not found.
        const std::uint32_t* m_EngineTlsIndex{ nullptr };

        static auto __fastcall HookedApplyPlayerActions(void* update) -> void;

        // Writes the override of every player that has one into the update, and marks those players as having an action.
        // param threadContext: Thread context of the engine, or 0 if unknown (then the views do not follow the overrides).
        auto ApplyOverrides(std::uint8_t* update, std::uintptr_t threadContext) -> void;

        // return: Thread context of the engine, or 0 if it is not found.
        // note: Reads the thread local storage of the current thread. Call it from the game thread.
        auto GetThreadContext() const -> std::uintptr_t;

        // return: Variable that holds the engine thread local storage index, or null if the signature is not found.
        auto FindEngineTlsIndex() -> const std::uint32_t*;

        // Copies the mask and the actions of the update to the store.
        auto PublishActions(const std::uint8_t* update) -> void;
    };
}