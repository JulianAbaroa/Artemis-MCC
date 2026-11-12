export module Engine.Base.MatchSetup.System:MatchSetup;

import Common.Log.System;
import Engine.Base.Lifecycle.State;
import Engine.Base.MapVariant.Type;
import Engine.Base.MapVariant.System;
import Engine.Base.MatchSetup.Type;
import Engine.Base.MatchSetup.State;
import Engine.Base.Preset.Type;
import Engine.Base.Preset.System;
import std;

export namespace Engine::Base::MatchSetup::System
{
    // What the setup service asks of the service that owns the bots (Engine.Control implements it): to check a roster against the match
    // and to apply it. Declared here because Engine.Base cannot see Engine.Control.
    // note: Game thread only.
    class RosterPort
    {
    private:
        using RosterApplication = Engine::Base::MatchSetup::Type::RosterApplication;
        using Roster = Engine::Base::MatchSetup::Type::Roster;

    public:
        virtual ~RosterPort() = default;

        // return: Why the roster does not fit the match as it is now (bot limit, free slots, team of the local player), or nothing.
        // Nothing is changed.
        virtual auto CheckRoster(std::uintptr_t threadContext, const Roster& roster) -> std::optional<std::string> = 0;

        // Does the moves, removes and adds that turn the bots of the match into the roster. A step that fails is logged, the ones done
        // stay done (no rollback) and the skipped ones count as not done.
        virtual auto ApplyRoster(std::uintptr_t threadContext, const Roster& roster) -> RosterApplication = 0;
    };

    // Applies a match setup (gametype preset, map variant and bots) whole or not at all (D286): at the start of the match or at a round
    // break. One request replaces the one waiting. Everything that can be checked is checked before anything is written, and the three
    // are written in the same call, in the order preset, map variant, bots.
    // note: The request thread resolves the names (files, blocks) and hands over values. The game thread never reads a disk. The round
    // break is applied by UpdateBreak, which SimulationTicksDetour calls before the ticks of every frame (it also runs while paused), and
    // the bots of the load by UpdateLoad, which PlayerActionDetour calls from the first tick of the match.
    // Rejected means nothing was applied. What the roster could not do after a preset or a map variant went in is Applied (AtLoad) with
    // the reason "partial: ...", because the roster has no rollback.
    // A request that fails its checks is Rejected, but a valid request that waited before stays and is still applied, with its own
    // result dropped as an older one.
    class MatchSetupService
    {
    private:
        using Log = Common::Log::System::Log;

        using LifecycleStore = Engine::Base::Lifecycle::State::LifecycleStore;
        using MapVariantService = Engine::Base::MapVariant::System::MapVariantService;
        using PresetService = Engine::Base::Preset::System::PresetService;

        using ActiveMapVariant = Engine::Base::MapVariant::Type::ActiveMapVariant;
        using ActiveMatchSetup = Engine::Base::MatchSetup::Type::ActiveMatchSetup;
        using MatchSetup = Engine::Base::MatchSetup::Type::MatchSetup;
        using MatchSetupOrigin = Engine::Base::MatchSetup::Type::MatchSetupOrigin;
        using MatchSetupRequest = Engine::Base::MatchSetup::Type::MatchSetupRequest;
        using MatchSetupResult = Engine::Base::MatchSetup::Type::MatchSetupResult;
        using PresetPick = Engine::Base::Preset::Type::PresetPick;
        using Roster = Engine::Base::MatchSetup::Type::Roster;
        using MatchSetupStore = Engine::Base::MatchSetup::State::MatchSetupStore;

    public:
        MatchSetupService(Log& log, LifecycleStore& lifecycleStore, MatchSetupStore& matchSetupStore, PresetService& presetService,
            MapVariantService& mapVariantService) :
            m_Log(log), m_LifecycleStore(lifecycleStore), m_MatchSetupStore(matchSetupStore), m_PresetService(presetService),
            m_MapVariantService(mapVariantService) {}
        ~MatchSetupService() = default;

        MatchSetupService(const MatchSetupService&) = delete;
        auto operator=(const MatchSetupService&) -> MatchSetupService& = delete;

        // Connects the service that owns the bots (Mod does it, before any hook is installed). Null disconnects it. A request with a
        // roster waits while there is none.
        auto SetRosterPort(RosterPort* port) -> void;

        // Asks for a setup. It waits for the next start or the next round break, whichever comes first, and replaces the request that was
        // waiting. A setup that can never fit is refused at once (Rejected, and the request that waited stays).
        // return: The number of the request. Its result is GetResult().Sequence.
        // note: While a match runs (or loads) the map variant and the preset catalog are compared with the ones of that match.
        // note: Any thread except the game thread.
        auto Request(MatchSetup setup, MatchSetupOrigin origin) -> std::uint64_t;

        // Records a request whose names could not be resolved (a file is missing, a block cannot be read). The result is Rejected and
        // nothing changes in the engine, and a request that waited stays.
        // return: The number of the request.
        auto Reject(std::string reason) -> std::uint64_t;

        // Drops the request that still waits if the given origin made it, when that client is gone.
        auto Cancel(MatchSetupOrigin origin) -> void;

        // return: The setup the engine holds, which every start applies again.
        auto GetActive() const -> ActiveMatchSetup;

        // return: Who made the request that waits, or nothing. The Workbench uses it so its buttons do not replace the request of
        // another client without saying so.
        auto GetPendingOrigin() const -> std::optional<MatchSetupOrigin>;

        // return: What became of the last request.
        auto GetResult() const -> MatchSetupResult;

        // return: Whether a start has something to apply: a request waits, or a preset, a map variant or bots are active.
        auto HasLoadWork() const -> bool;

        // Starts the per-start state, for every start: drops the base saved for the match that ended and keeps the map variant the
        // configuration carries.
        // param configuration: The configuration the engine starts with, before anything is written into it.
        // note: Called by the start hook on the MCC thread, before ApplyToStart. Never throws.
        auto BeginStart(std::span<const std::byte> configuration) -> void;

        // Writes the setup of this start into the configuration: the request waiting (or the setup the engine holds) with its preset and
        // map variant, both or none. It becomes the active setup if it fits. The bots are left for the first tick (UpdateLoad). A setup
        // that does not fit leaves the configuration as it was, is Rejected, and the start gets the setup the engine held before.
        // param configuration: The whole configuration the engine starts with. It has to be a copy: the one of MCC is never written.
        // note: Called by the start hook on the MCC thread. Never throws. A start with no request has no result to give.
        auto ApplyToStart(std::span<std::byte> configuration) -> void;

        // Applies the bots of the start at the first tick of the match, and marks that a request that waits then is part of the load.
        // note: Game thread only, from the first tick (PlayerActionDetour). Never throws. Does nothing after the first tick.
        auto UpdateLoad(std::uintptr_t threadContext) -> void;

        // Applies the request waiting at the first frame whose round is ended or between rounds, never in the middle of a round (or in the
        // first frame after the load when it waited at the first tick). An immediate request (MatchSetup::IsImmediate, roster only) is
        // applied in the first frame after the first tick whatever the round state, through the same RosterPort the break uses. The request is taken once and ends as Applied (AtLoad) or Rejected.
        // note: Game thread only, from a hook that runs before the ticks of every frame, paused or not (SimulationTicksDetour). Never
        // throws.
        auto UpdateBreak(std::uintptr_t threadContext) -> void;

    private:
        Log& m_Log;
        LifecycleStore& m_LifecycleStore;
        MatchSetupStore& m_MatchSetupStore;
        PresetService& m_PresetService;
        MapVariantService& m_MapVariantService;
        std::atomic<RosterPort*> m_RosterPort{ nullptr };

        // Writes preset and map variant into the copy of the configuration.
        // param isPresetRequired: A preset that does not apply to the variant of the start (not Megalo) is a failure and not a no-op.
        // return: Why it could not be written, or nothing. The configuration and the preset base are as they were on a failure.
        auto WriteToStart(std::span<std::byte> configuration, const PresetPick& preset, const std::shared_ptr<const ActiveMapVariant>& mapVariant,
            bool isPresetRequired) -> std::optional<std::string>;

        // Applies the bots of the start at the first tick.
        auto ApplyLoad(std::uintptr_t threadContext) -> void;

        // Serves the request waiting, if the moment is right for what it holds.
        // param isBreak: Whether this is a tick of a round break or of the load. Out of a break only an immediate request is served, and
        // that is decided on the request taken, under the lock of the store.
        auto ApplyBreak(std::uintptr_t threadContext, bool isAtLoad, bool isBreak) -> void;

        // Checks everything the request holds against the live match, then writes it, and records the result.
        auto Serve(std::uintptr_t threadContext, RosterPort* port, const MatchSetupRequest& request, bool isAtLoad) -> void;
    };
}