export module Bridge.Codec.Flat.Type;

import Common.Engine.Type;
import Contract.Bridge.Type;
import Bridge.Codec.Tick.Type;
import std;

namespace
{
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
    using Contract::Bridge::Type::BridgeAgentInput;
}

export namespace Bridge::Codec::Flat::Type
{
    // Action tick and applied decisions read while the game thread waits for the read of a tick, so they match the state of the
    // tables copied in that tick.
    struct BridgeClock
    {
        // Last action tick the engine applied before this state.
        std::uint64_t ActionTick{ 0 };

        // DecisionTick of the bridge action applied to each player (by player table index), 0 = none.
        std::array<std::uint64_t, k_MaxPlayers> AppliedDecisions{};

        // The player the user possesses and the last closed window of what it did, as an AgentInput (D364), read together on the game thread.
        // NoHandle and zero when none is possessed.
        std::uint32_t PossessedPlayer{ k_NoHandle };
        BridgeAgentInput PossessedInput{};

        // QueryPerformanceCounter marks of the tick, one per hand-off between threads (P.3 C, D373). 0 = not set.
        // CaptureQpc: the game thread started to capture. AiStartQpc: the AI thread took the tick out of the tick queue. AiEndQpc: the AI
        // thread queued it for the bridge thread. FlattenStartQpc: the bridge thread took it out of its queue.
        std::uint64_t CaptureQpc{ 0 };
        std::uint64_t AiStartQpc{ 0 };
        std::uint64_t AiEndQpc{ 0 };
        std::uint64_t FlattenStartQpc{ 0 };

        // Last decision the game thread needed in Lockstep up to the capture, when it arrived at its tick plus the latency and when the
        // decision was available (BridgeHookState.Need*), read on the game thread. NeedFlags are BridgeNeedFlag bits. 0 = none yet.
        std::uint64_t NeedDecision{ 0 };
        std::uint64_t NeedQpc{ 0 };
        std::uint64_t NeedEndQpc{ 0 };
        std::uint32_t NeedFlags{ 0 };

        // Whether this state is a decision tick of a connected client (a tick goes out).
        bool IsDecision{ false };

        // Whether the pipeline runs only on decision ticks (Q101): a tick that is not a decision does the light pass only.
        bool IsSparse{ false };
    };

    // A Tick of a decision tick waiting to be flattened into a BridgeTick, with the clock read with it (D135).
    struct FlatRequest
    {
        std::shared_ptr<const Bridge::Codec::Tick::Type::Tick> Snapshot{};
        BridgeClock Clock{};
    };
}