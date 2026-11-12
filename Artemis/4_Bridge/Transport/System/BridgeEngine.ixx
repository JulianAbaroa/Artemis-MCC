export module Bridge.Transport.System:Engine;

import Control.Action.Type;
import Bridge.Transport.Type;
import std;

export namespace Bridge::Transport::System
{
    // What the bridge reads and writes in the engine (D215). Implemented by the owner (Mod), the bridge never imports the engine.
    // note: The bridge calls it from its own thread (Update, EndTick), from the AI thread (CaptureClock) and from the game thread
    // (Apply, and BeginMatch when the engine initializes).
    class BridgeEngine
    {
    private:
        using ActionOverride = Control::Action::Type::ActionOverride;
        using ClientRequests = Control::Action::Type::ClientRequests;

        using BridgePhase = Contract::Bridge::Type::BridgePhase;
        using BridgeActiveSetup = Bridge::Transport::Type::BridgeActiveSetup;
        using BridgeSetupInfo = Bridge::Transport::Type::BridgeSetupInfo;
        using BridgeSetupRequest = Bridge::Transport::Type::BridgeSetupRequest;

    public:
        virtual ~BridgeEngine() = default;

        // return: Phase of the engine for the client: loading, running or idle.
        virtual auto GetPhase() const -> BridgePhase = 0;

        // return: Number of times the engine has started, the id of the current match.
        virtual auto GetEngineStarts() const -> std::uint64_t = 0;

        // return: Frames the engine has spent paused.
        virtual auto GetPausedFrames() const -> std::uint64_t = 0;

        virtual auto GetRequestedTimeScale() const -> float = 0;
        virtual auto SetRequestedTimeScale(float scale) -> void = 0;

        virtual auto IsPauseRequested() const -> bool = 0;
        virtual auto SetPauseRequested(bool value) -> void = 0;

        virtual auto SetEndRoundRequested(bool value) -> void = 0;

        // Asks for one match setup: a gametype preset, a map variant and a bot roster, whole or not at all (LS.2). In the lobby it is
        // written at the next start, in a match at the next round break. The names are resolved here, on the bridge thread.
        virtual auto RequestSetup(const BridgeSetupRequest& request) -> void = 0;

        // Drops the request that still waits for a round break, when the client that made it is gone.
        virtual auto CancelSetupRequest() -> void = 0;

        // return: What became of the last request.
        virtual auto GetSetupResult() const -> BridgeSetupInfo = 0;

        // return: The setup the engine holds.
        virtual auto GetActiveSetup() const -> BridgeActiveSetup = 0;

        // Sets the override of a player for the next action, or none to give the player back to the engine.
        virtual auto SetOverride(std::size_t playerIndex, std::optional<ActionOverride> actionOverride) -> void = 0;

        // Asks the engine for the weapon, grenade and interaction requests of a client player.
        virtual auto RequestClient(std::size_t playerIndex, const ClientRequests& requests) -> void = 0;
    };
}