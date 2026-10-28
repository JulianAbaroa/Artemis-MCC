export module Runtime.Thread:AI;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Resolved.Layer;
import Layout.Snapshot.Type;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import Export.Flat.Type;
import Export.Pipeline.Type;
import Export.Pipeline.System;
import std;

namespace
{
    using Export::Pipeline::System::k_PipelineStageNames;
}

export namespace Runtime::Thread
{
    // Thread that loads the map resources and runs the simulation tick pipeline.
    // note: Runs until the lifecycle stops and blocks on the lifecycle events while idle.
    class AIThread
    {
    private:
        using steady_clock = std::chrono::steady_clock;

        using BridgeClock = Export::Flat::Type::BridgeClock;

        using Snapshot = Layout::Snapshot::Type::Snapshot;

        using ObservedTick = Export::Pipeline::Type::ObservedTick;

    public:
        AIThread(Service::Layer& service, Platform::Layer& platform, Map::Layer& map,
            Resolved::Layer& resolved, Tables::Layer& tables, Relations::Layer& relations,
            Environment::Layer& environment, Egocentric::Layer& egocentric,
            Export::Layer& exportLayer) :
            m_Service(service), m_Platform(platform), m_Map(map),
            m_Resolved(resolved), m_Tables(tables), m_Relations(relations),
            m_Environment(environment), m_Egocentric(egocentric),
            m_Export(exportLayer) {}
        ~AIThread() = default;

        // Runs the thread loop until the lifecycle stops.
        auto Run() -> void;

        // Adds a function called every tick after the pipeline and the bridge push, on this thread.
        // note: Only before Run. There is no lock.
        auto AddObserver(std::function<void(const ObservedTick&)> observer) -> void;

    private:
        Service::Layer& m_Service;
        Platform::Layer& m_Platform;
        Map::Layer& m_Map;
        Resolved::Layer& m_Resolved;
        Tables::Layer& m_Tables;
        Relations::Layer& m_Relations;
        Environment::Layer& m_Environment;
        Egocentric::Layer& m_Egocentric;
        Export::Layer& m_Export;
        std::vector<std::function<void(const ObservedTick&)>> m_Observers{};

        // Stages of a sweep, in the order they run. Timed to find the one behind a slow sweep.
        static constexpr std::array<std::string_view, k_PipelineStageNames.size() + 4> k_StageNames{ [] {
            std::array<std::string_view, k_PipelineStageNames.size() + 4> names{ "Capture" };
            std::ranges::copy(k_PipelineStageNames, names.begin() + 1);
            names[k_PipelineStageNames.size() + 1] = "Bridge";
            names[k_PipelineStageNames.size() + 2] = "Observers";
            names.back() = "Other";
            return names;
        }() };

        bool m_IsLoaded{ false };
        // The next tick that runs the pipeline is the first one after the map loaded (D207).
        bool m_IsFirstTickAfterLoad{ true };

        std::uint64_t m_Last{ 0 };
        std::uint64_t m_Dropped{ 0 };

        // Clock of the bridge read with the current tick (D135): its action tick and whether a frame goes to Python.
        BridgeClock m_BridgeClock{};

        // Time of each stage in the current sweep, in nanoseconds.
        std::array<std::uint64_t, k_StageNames.size()> m_StageNs{};
        // Next stage to record.
        std::size_t m_Stage{ 0 };
        steady_clock::time_point m_StageStart{};
        // Calls EndStage. Built once so the pipeline takes it without allocating every tick.
        std::function<void()> m_EndStage{ [this] { this->EndStage(); } };

        // Builds the map, definitions, world and vitality data once the map file is loaded.
        auto LoadResources() -> void;

        // Copies the game into a Snapshot, releases the game thread and runs the pipeline over the Snapshot.
        // return: The Snapshot the tick was built from.
        auto ExecuteTick(std::uint64_t generation) -> std::shared_ptr<const Snapshot>;

        // Records the time since the previous stage ended as the next stage of k_StageNames.
        auto EndStage() -> void;

        // Logs the time of every stage when a sweep is slow enough to stall the game.
        // note: The game waits for the read of each tick for up to 100 ms (LifecycleStore), so a sweep near that drops ticks.
        auto ReportSlowSweep(std::uint64_t sweepNs) -> void;

        // Checks whether the lifecycle is running and not tearing down.
        auto IsStable() -> bool;

        // Clears the tick generation and the load state for the next map.
        auto Reset() -> void;
    };
}