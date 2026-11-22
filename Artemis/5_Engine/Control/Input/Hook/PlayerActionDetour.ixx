export module Engine.Control.Input.Hook:PlayerAction;

import Common.Log.System;
import Control.Action.Type;
import Engine.Base.MatchSetup.System;
import Engine.Base.Memory.System;
import Engine.Control.Input.Type;
import Engine.Control.Input.State;
import Engine.Control.Input.System;
import Engine.Control.Simulation.System;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
}

export namespace Engine::Control::Input::Hook
{
    // Hooks the engine function that applies the player actions of each tick: writes the overrides into them, then publishes them (D109).
    // note: Players without an override keep the action the engine built. Called by ExecuteGameFrame once per tick on the game thread.
    // Before the actions, the match setup service applies the setup asked for (D286: preset, map variant and bots, at the first tick and
    // at the break between rounds), the bot service adds the bots asked for and publishes the session snapshot (A9 M5), and the action source
    // (the bridge, D215) sets the overrides of the players of the Python client (waiting for them in Lockstep, D105).
    // note: The action tick of the bridge is the number this hook publishes (m_Sequence): one per tick it sees.
    class PlayerActionDetour
    {
    private:
        using Log = Common::Log::System::Log;

        using MatchSetupService = Engine::Base::MatchSetup::System::MatchSetupService;
        using AOBService = Engine::Base::Memory::System::AOBService;

        using ActionOverride = ::Control::Action::Type::ActionOverride;
        using ClientRequests = ::Control::Action::Type::ClientRequests;
        using PlayerActionHistory = ::Control::Action::Type::PlayerActionHistory;

        using ApplyPlayerActionsFunction = Engine::Control::Input::Type::ApplyPlayerActionsFunction;
        using ClearPlayerActionFunction = Engine::Control::Input::Type::ClearPlayerActionFunction;
        using ControlTagLocator = Engine::Control::Input::Type::ControlTagLocator;
        using CopyActionToViewFunction = Engine::Control::Input::Type::CopyActionToViewFunction;
        using ShadowField = Engine::Control::Input::Type::ShadowField;
        using ShadowPlayer = Engine::Control::Input::Type::ShadowPlayer;

        using PlayerActionStore = Engine::Control::Input::State::PlayerActionStore;
        using PossessionStore = Engine::Control::Input::State::PossessionStore;
        using PossessRequestStore = Engine::Control::Input::State::PossessRequestStore;

        using ActionSource = Engine::Control::Input::System::ActionSource;

        using BotService = Engine::Control::Simulation::System::BotService;

    public:
        PlayerActionDetour(Log& log, AOBService& aobService, MatchSetupService& matchSetupService, PlayerActionStore& playerActionStore,
            BotService& botService) :
            m_Log(log), m_AOBService(aobService), m_MatchSetupService(matchSetupService), m_PlayerActionStore(playerActionStore),
            m_BotService(botService) {}
        ~PlayerActionDetour() = default;

        // Connects the action source (the bridge, D215) that sets the overrides of the Python players.
        // note: The one that composes the layers calls it before Install. Runs once, on the thread that builds the layers (D243).
        auto SetActionSource(ActionSource& actionSource) -> void;

        // Connects the request of the bot to possess and the state of the possession (VW.9c.2 B).
        // note: The one that composes the layers calls it before Install, like SetActionSource. Without it no bot is possessed.
        auto SetPossession(PossessRequestStore& requestStore, PossessionStore& possessionStore) -> void;

        // Finds the target by signature and installs the hook. Does nothing if already installed.
        // return: False if there is no action source, the target is not found or the hook fails.
        auto Install() -> bool;

        // Waits for the calls in flight to drain, then removes the hook. Does nothing if not installed.
        auto Uninstall() -> void;

    private:
        Log& m_Log;
        AOBService& m_AOBService;
        MatchSetupService& m_MatchSetupService;
        PlayerActionStore& m_PlayerActionStore;
        BotService& m_BotService;

        // The bot the Viewer asks to possess (VW.9c.2 B) and what the client of the user keeps for it. Null until connected: then nobody is possessed.
        PossessRequestStore* m_PossessRequestStore{ nullptr };
        PossessionStore* m_PossessionStore{ nullptr };
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

        // Where the player control tag is (the shared locator). Invalid if not found (then no lane is ever held).
        ControlTagLocator m_ControlTag{};

        // Engine function that clears an action, or null if not found (then no override is applied).
        ClearPlayerActionFunction m_ClearPlayerAction{ nullptr };

        // Engine function that copies an action into the input of a view, or null if not found (then the view of a non local player
        // does not turn).
        CopyActionToViewFunction m_CopyActionToView{ nullptr };

        // Variable of the engine that holds the thread local storage index of its thread context, or null if not found.
        const std::uint32_t* m_EngineTlsIndex{ nullptr };

        static auto __fastcall HookedApplyPlayerActions(void* update) -> void;

        // Writes the action of every player that has an override into the update, and marks those players as having an action.
        // note: The action starts from the empty action of the engine, then the unit slot, the override and the interaction (D119).
        // param threadContext: Thread context of the engine, or 0 if unknown (then no override is applied).
        auto ApplyOverrides(std::uint8_t* update, std::uintptr_t threadContext) -> void;

        // Possession of a bot (VW.9c.2 B, D350, D360): while the Viewer asks for one, the tick copies what the client of the user accumulated in the
        // frames (the 0x5df80 hook runs it, see PossessionService) into the action of the bot through the same override path as an agent, and the
        // human local gets a neutral action.
        // param engineMask: Players with an action from the engine this tick.
        // param overrides: The overrides of the tick. The one of the possessed bot is replaced (the override of Python is dropped, D351) and the
        // human local gets a neutral one, unless it is an agent (D350).
        // param requests: The client requests of the tick, replaced the same way.
        // note: Game thread only, once per tick, after the overrides were copied. Without a request, or when the bot is gone or is not an Artemis
        // bot (never a human, D352), the possession is released and nothing changes.
        auto ApplyPossession(std::uintptr_t threadContext, std::uint32_t engineMask,
            std::array<std::optional<ActionOverride>, k_MaxActionPlayers>& overrides, std::array<ClientRequests, k_MaxActionPlayers>& requests)
            -> void;

        // Does what the engine does in the theater for the views that show a non local player: copies the action of that player into the
        // input of the view, so the camera, zoom and weapon follow a bot tick by tick (VW.9a.y, D349).
        // note: Game thread only, after the engine applied the actions. Resolves every pointer from the thread context each tick. Does
        // nothing in the theater, without the function, or without the thread context, the mapping or the game globals.
        auto CopyActionsToViews(const std::uint8_t* update, std::uintptr_t threadContext) -> void;

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

        // return: Variable that holds the engine thread local storage index, or null if the signature is not found.
        auto FindEngineTlsIndex() -> const std::uint32_t*;

        // Copies the mask and the actions of the update to the store.
        auto PublishActions(const std::uint8_t* update) -> void;
    };
}