export module Platform.Input.Hook:PlayerAction;

import Common.Log.System;
import Control.Action.Type;
import Platform.Memory.System;
import Platform.Input.Type;
import Platform.Input.State;
import Platform.Input.System;
import Platform.Simulation.System;
import Layout.Action.Type;
import std;

namespace
{
    using Layout::Action::Type::Size::k_MaxActionPlayers;
}

export namespace Platform::Input::Hook
{
    // Hooks the engine function that applies the player actions of each tick: writes the overrides into them, then publishes them (D109).
    // note: Players without an override keep the action the engine built. Called by ExecuteGameFrame once per tick on the game thread.
    // Before the actions, the bot service adds the bots asked for and publishes the session snapshot (A9 M5), and the action source
    // (the bridge, D215) sets the overrides of the players of the Python client (waiting for them in Lockstep, D105).
    // note: The action tick of the bridge is the number this hook publishes (m_Sequence): one per tick it sees.
    class PlayerActionDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using AOBService = Platform::Memory::System::AOBService;

        using ActionOverride = Control::Action::Type::ActionOverride;
        using ClientRequests = Control::Action::Type::ClientRequests;
        using PlayerActionHistory = Control::Action::Type::PlayerActionHistory;

        using ApplyPlayerActionsFunction = Platform::Input::Type::ApplyPlayerActionsFunction;
        using ClearPlayerActionFunction = Platform::Input::Type::ClearPlayerActionFunction;
        using ResolveTagAddressFunction = Platform::Input::Type::ResolveTagAddressFunction;
        using ShadowField = Platform::Input::Type::ShadowField;
        using ShadowPlayer = Platform::Input::Type::ShadowPlayer;

        using PlayerActionStore = Platform::Input::State::PlayerActionStore;

        using ActionSource = Platform::Input::System::ActionSource;

        using BotService = Platform::Simulation::System::BotService;

    public:
        PlayerActionDetour(Log& log, AOBService& aobService, PlayerActionStore& playerActionStore,
            BotService& botService) :
            m_Log(log), m_AOBService(aobService), m_PlayerActionStore(playerActionStore), m_BotService(botService) {}
        ~PlayerActionDetour() = default;

        // Connects the action source (the bridge, D215) that sets the overrides of the Python players.
        // note: The one that composes the layers calls it before Install. Runs once, on the thread that builds the layers (D243).
        auto SetActionSource(ActionSource& actionSource) -> void;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if there is no action source, the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        PlayerActionStore& m_PlayerActionStore;
        BotService& m_BotService;
        ActionSource* m_ActionSource{ nullptr };

        static inline PlayerActionDetour* s_Instance{ nullptr };
        static inline ApplyPlayerActionsFunction s_OriginalFunction{ nullptr };

        // Calls currently inside the hook. Uninstall waits for it to reach zero.
        static inline std::atomic<int> s_InFlight{ 0 };

        std::atomic<void*> m_FunctionAddress{ nullptr };
        std::atomic<bool> m_IsHookInstalled{ false };

        // Ticks seen since the hook was installed. Game thread only.
        std::uint64_t m_Sequence{ 0 };

        // What the action of each agent remembers between ticks (D213): the weapon it asked for and does not hold yet (D120), the ticks
        // each hold lane has been down (D121) and the facing it got on its last tick, where its turn goes on from while its unit is the
        // same (D114). Forgotten without an override. Game thread only.
        std::array<PlayerActionHistory, k_MaxActionPlayers> m_Histories{};

        // What the shadow check remembers of each local player (D122). Reset while the check is off. Game thread only.
        std::array<ShadowPlayer, k_MaxActionPlayers> m_ShadowPlayers{};

        // Variable of the engine that points to the globals table with the player control tag, and the engine function that resolves
        // a tag address. Null if not found (then no lane is ever held).
        const std::uintptr_t* m_PlayerControlTable{ nullptr };
        ResolveTagAddressFunction m_ResolveTagAddress{ nullptr };

        // Engine function that clears an action, or null if not found (then no override is applied).
        ClearPlayerActionFunction m_ClearPlayerAction{ nullptr };

        // Variable of the engine that holds the thread local storage index of its thread context, or null if not found.
        const std::uint32_t* m_EngineTlsIndex{ nullptr };

        static auto __fastcall HookedApplyPlayerActions(void* update) -> void;

        // Writes the action of every player that has an override into the update, and marks those players as having an action.
        // note: The action starts from the empty action of the engine, then the unit slot, the override and the interaction (D119).
        // param threadContext: Thread context of the engine, or 0 if unknown (then no override is applied).
        auto ApplyOverrides(std::uint8_t* update, std::uintptr_t threadContext) -> void;

        // Shadow check (D122): for every local player without an override, compares the client fields Artemis would send for its
        // unit with the ones the engine built this tick, and counts the results in the store.
        // note: Game thread only. Development tool, off by default. Mismatches near the hold threshold within one tick count as
        // matches: the engine counts milliseconds of real frames.
        auto CheckShadow(const std::uint8_t* update, std::uintptr_t threadContext, std::uint32_t engineMask,
            const std::array<std::optional<ActionOverride>, k_MaxActionPlayers>& overrides, std::optional<std::uint32_t> holdTicks)
            -> void;

        // Counts one comparison and logs the first mismatches of each field.
        auto ReportShadow(ShadowField field, std::size_t player, bool isMatch, std::int64_t engineValue, std::int64_t artemisValue)
            -> void;

        // return: The hold threshold of buttons in ticks, read from the player control tag of the engine globals, or nullopt if it
        // cannot be read.
        // note: Game thread only. Read every tick: the globals belong to the map.
        auto GetHoldThresholdTicks() const -> std::optional<std::uint32_t>;

        // return: Thread context of the engine, or 0 if it is not found.
        // note: Reads the thread local storage of the current thread. Call it from the game thread.
        auto GetThreadContext() const -> std::uintptr_t;

        // return: Variable that holds the engine thread local storage index, or null if the signature is not found.
        auto FindEngineTlsIndex() -> const std::uint32_t*;

        // Copies the mask and the actions of the update to the store.
        auto PublishActions(const std::uint8_t* update) -> void;
    };
}