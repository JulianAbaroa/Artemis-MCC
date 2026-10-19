export module Platform.Input.Hook:PlayerAction;

import Service.Logs.System;
import Platform.Memory.System;
import Platform.Input.Type;
import Platform.Input.State;
import Platform.Simulation.System;
import std;

namespace
{
    using Platform::Input::Type::k_MaxActionPlayers;
    using Platform::Input::Type::k_HoldLanes;
}

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
        using ClearPlayerActionFunction = Platform::Input::Type::ClearPlayerActionFunction;
        using ClientRequests = Platform::Input::Type::ClientRequests;
        using WeaponRequest = Platform::Input::Type::WeaponRequest;
        using ResolveTagAddressFunction = Platform::Input::Type::ResolveTagAddressFunction;

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

        // Weapon slot each agent asked for and does not hold yet (D120). Game thread only.
        std::array<std::optional<WeaponRequest>, k_MaxActionPlayers> m_WeaponRequests{};

        // Ticks each hold lane of each agent has been down (D121). Game thread only.
        std::array<std::array<std::uint32_t, k_HoldLanes.size()>, k_MaxActionPlayers> m_LaneTicks{};

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

        // Writes the hold lanes of the agent into its action: the down bits it holds, plus the held bit of every lane down for the
        // hold threshold of the engine (D121).
        // param holdTicks: Hold threshold in ticks, or nullopt if unknown (then no held bit is set).
        // note: Game thread only. Counted in ticks of game time, so the speed of the engine does not change it.
        auto WriteLanes(std::uint8_t* action, std::size_t player, std::uint32_t lanes, std::optional<std::uint32_t> holdTicks)
            -> void;

        // return: The hold threshold of buttons in ticks, read from the player control tag of the engine globals, or nullopt if it
        // cannot be read.
        // note: Game thread only. Read every tick: the globals belong to the map.
        auto GetHoldThresholdTicks() const -> std::optional<std::uint32_t>;

        // Writes the client requests of the agent (weapon, grenade, zoom) into its action, with the engine selectors (D120).
        // note: Without a request the action asks for nothing and keeps the zoom of the unit. For a local player with an override the
        // zoom its own input asks for is ignored. A weapon request is sent again every tick until the unit holds that slot,
        // k_WeaponRequestTicks pass or the inventory changes. Game thread only.
        auto WriteClientRequests(std::uint8_t* action, std::uintptr_t threadContext, std::size_t player,
            const ClientRequests& requests) -> void;

        // return: Thread context of the engine, or 0 if it is not found.
        // note: Reads the thread local storage of the current thread. Call it from the game thread.
        auto GetThreadContext() const -> std::uintptr_t;

        // return: Variable that holds the engine thread local storage index, or null if the signature is not found.
        auto FindEngineTlsIndex() -> const std::uint32_t*;

        // Copies the mask and the actions of the update to the store.
        auto PublishActions(const std::uint8_t* update) -> void;
    };
}