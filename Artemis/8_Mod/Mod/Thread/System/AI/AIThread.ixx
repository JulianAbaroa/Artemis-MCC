export module Mod.Thread.System:AI;

import Services.Layer;
import Engine.Base.Layer;
import Engine.Capture.Layer;
import Bridge.Transport.Layer;
import Blam.Map.Layer;
import Blam.MapModel.Layer;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Layer;
import Perception.Layer;
import Views.Layer;
import Views.View.Type;
import Bridge.Codec.Layer;
import Bridge.Codec.Pipeline.Type;
import Bridge.Codec.Pipeline.System;
import std;

namespace
{
    using Bridge::Codec::Pipeline::System::k_PipelineStageNames;
}

export namespace Mod::Thread::System
{
    // Thread that loads the map resources and runs the simulation tick pipeline over the ticks the game thread queued (D305).
    // note: Runs until the lifecycle stops and blocks on the lifecycle events while idle. It never touches game memory: the game thread
    // copies each tick into a Snapshot and queues it, and this thread takes the items in order.
    class AIThread
    {
    private:
        using steady_clock = std::chrono::steady_clock;

        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;
        using AgentSet = Views::View::Type::AgentSet;

        using CapturedTick = Bridge::Codec::Pipeline::Type::CapturedTick;
        using ObservedTick = Bridge::Codec::Pipeline::Type::ObservedTick;
        using TickQueue = Bridge::Codec::Pipeline::Type::TickQueue;

    public:
        AIThread(Services::Layer& service, Engine::Base::Layer& base, Engine::Capture::Layer& capture,
            Bridge::Transport::Layer& transport, Blam::Map::Layer& map,
            Blam::MapModel::Layer& resolved, Blam::Runtime::Layer& tables,
            Perception::Layer& environment, Views::Layer& egocentric,
            Bridge::Codec::Layer& exportLayer, TickQueue& tickQueue) :
            m_Service(service), m_Base(base), m_Capture(capture), m_Transport(transport), m_Map(map),
            m_Resolved(resolved), m_Tables(tables),
            m_Environment(environment), m_Egocentric(egocentric),
            m_Export(exportLayer), m_TickQueue(tickQueue) {}
        ~AIThread() = default;

        // Runs the thread loop until the lifecycle stops.
        auto Run() -> void;

        // Adds a function called every tick after the pipeline and the bridge push, on this thread.
        // note: Only before Run. There is no lock.
        auto AddObserver(std::function<void(const ObservedTick&)> observer) -> void;

    private:
        Services::Layer& m_Service;
        Engine::Base::Layer& m_Base;
        Engine::Capture::Layer& m_Capture;
        Bridge::Transport::Layer& m_Transport;
        Blam::Map::Layer& m_Map;
        Blam::MapModel::Layer& m_Resolved;
        Blam::Runtime::Layer& m_Tables;
        Perception::Layer& m_Environment;
        Views::Layer& m_Egocentric;
        Bridge::Codec::Layer& m_Export;
        TickQueue& m_TickQueue;
        std::vector<std::function<void(const ObservedTick&)>> m_Observers{};

        // Longest wait for the next queued tick. The thread looks at the lifecycle again after it, so a teardown or a stop is noticed.
        static constexpr std::chrono::milliseconds k_PopTimeout{ 10 };

        // Stages of a sweep, in the order they run. Timed to find the one behind a slow sweep. The capture is not one of them: the game thread
        // does it (D305) and the telemetry times it apart.
        static constexpr std::array<std::string_view, k_PipelineStageNames.size() + 3> k_StageNames{ [] {
            std::array<std::string_view, k_PipelineStageNames.size() + 3> names{};
            std::ranges::copy(k_PipelineStageNames, names.begin());
            names[k_PipelineStageNames.size()] = "Bridge";
            names[k_PipelineStageNames.size() + 1] = "Observers";
            names.back() = "Other";
            return names;
        }() };

        bool m_IsLoaded{ false };
        // Whether the queue is open, that is, this thread consumes ticks. The game thread queues only then.
        bool m_IsConsuming{ false };

        // Teardown epoch (LifecycleStore) the state was last reset for.
        std::uint64_t m_ResetEpoch{ 0 };

        // Generation of the last tick taken from the queue, for the logs.
        std::uint64_t m_Last{ 0 };
        // Teardown epoch of the tick being processed.
        std::uint64_t m_TickEpoch{ 0 };

        // Time of each stage in the current sweep, in nanoseconds.
        std::array<std::uint64_t, k_StageNames.size()> m_StageNs{};
        // Next stage to record.
        std::size_t m_Stage{ 0 };
        steady_clock::time_point m_StageStart{};
        // Calls EndStage. Built once so the pipeline takes it without allocating every tick.
        std::function<void()> m_EndStage{ [this] { this->EndStage(); } };
        // Asks the lifecycle whether the tick may still publish, for the pipeline right before it assembles the Tick.
        std::function<bool()> m_CanPublish{ [this] { return this->IsCurrent(); } };

        // Builds the map, definitions, world and vitality data once the map file is loaded.
        auto LoadResources() -> void;

        // Opens or closes the queue: the game thread queues ticks only while it is open.
        auto SetConsuming(bool value) -> void;

        // return: The players that get a View in this tick (D307). With a client that owns the instance, the players of its AgentMask and the
        // local player 0. Without one, the local player 0. In both, the player the Viewer follows (ObservedPlayerStore of Services, which the
        // Viewer writes: the AI thread imports no tool).
        // note: The local player is the one of the last published player table, so it is missing in the first tick after a load.
        auto BuildAgentSet() const -> AgentSet;

        // Runs the pipeline over the Snapshot of a queued tick.
        // return: False if the engine started tearing down before or while it ran (the pipeline did not complete, nothing is published).
        auto ExecuteTick(const CapturedTick& tick) -> bool;

        // Sparse mode, a tick that is not a decision: runs the light pass (the flag pass) over the light Snapshot. Nothing is published and
        // no observer runs.
        // return: False if the engine started tearing down before or while it ran (the pass did not complete).
        auto ExecuteLightTick(const CapturedTick& tick) -> bool;

        // Takes a queued tick through the pipeline, hands it to the bridge on a decision and to the observers, and records the telemetry.
        auto ProcessTick(const CapturedTick& tick) -> void;

        // Counts a tick that was cut by a teardown (shown in the log of the teardown) and logs it.
        auto DiscardTick(std::uint64_t generation) -> void;

        // Drops the ticks still in the queue, counted as discarded.
        // note: Only called when the teardown epoch changed: the game thread queues nothing for the old epoch any more.
        auto DiscardPending() -> void;

        // Records the time since the previous stage ended as the next stage of k_StageNames.
        auto EndStage() -> void;

        // Logs the time of every stage when a sweep is slow enough to stall the game.
        // note: Runs after Run recorded the sweep, so the last stage (Other) is already filled.
        // note: The game waits for the read of each tick for up to 100 ms (LifecycleStore), so a sweep near that drops ticks.
        auto ReportSlowSweep(std::uint64_t sweepNs) -> void;

        // Checks whether the tick being processed may still publish: the engine is not tearing down and the tick belongs to the current
        // teardown epoch, so a tick that outlived the wait of the game thread never publishes into the stores of the next map.
        auto IsCurrent() const -> bool;

        // Checks whether the lifecycle is running and not tearing down.
        auto IsStable() -> bool;

        // Closes the queue, drops its ticks and clears the load state for the next map.
        // note: Idempotent: Run calls it when the teardown epoch changed and again in TearingDown.
        auto Reset() -> void;
    };
}