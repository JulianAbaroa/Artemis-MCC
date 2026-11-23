export module Bridge.Transport.System:Engine;

import Control.Client.Type;
import Bridge.Transport.Type;
import std;

export namespace Bridge::Transport::System
{
    // What the bridge reads and writes in the engine (D215). Implemented by the owner (Mod), the bridge never imports the engine.
    // note: The bridge calls it from its own thread (Update, EndTick), and from the game thread (CaptureClock, Apply, and BeginMatch
    // when the engine initializes).
    class BridgeEngine
    {
    private:
        using AgentInput = Control::Client::Type::AgentInput;

        using BridgePhase = Contract::Bridge::Type::BridgePhase;
        using BridgeActiveSetup = Bridge::Transport::Type::BridgeActiveSetup;
        using BridgeImitation = Bridge::Transport::Type::BridgeImitation;
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

        // Gives a player a window of AgentInput: the client of Artemis plays it every tick (D363) and repeats the last one until the next.
        // param playerHandle: Handle of the player in the slot, so the engine drops the input when the slot has another player.
        // param input: Already sane (ToAgentInput).
        // param windowTicks: Ticks of the window (d).
        // param turnLimit: Radians per tick and per axis the look may ask, 0 = no limit.
        virtual auto SetAgentInput(std::size_t playerIndex, std::uint32_t playerHandle, const AgentInput& input, std::uint32_t windowTicks,
            float turnLimit) -> void = 0;

        // Stops the AgentInput of a player: the engine action is its own again.
        virtual auto ClearAgentInput(std::size_t playerIndex) -> void = 0;

        // Closes the window of the possessed player, if any: what it did since the previous close becomes the last window (D364).
        // note: Game thread, once per action tick, where the window ends (IsImitationWindowEnd).
        virtual auto CloseImitationWindow() -> void = 0;

        // return: The possessed player and the last closed window of what it did, read together (NoHandle and zero when none is possessed).
        // note: Game thread, when the clock of a tick is read, so the handle and the window are of the same moment.
        virtual auto GetImitation() const -> BridgeImitation = 0;
    };
}