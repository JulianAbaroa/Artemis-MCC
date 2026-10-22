export module Runtime.Thread:AI;

import Service.Layer;
import Platform.Layer;
import Platform.Bridge.Type;
import Map.Layer;
import Resolved.Layer;
import Datum.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import Viewer.Camera.State;
import UI.TickTrace.System;
import std;

export namespace Runtime::Thread
{
    // Thread that loads the map resources and runs the simulation tick pipeline.
    // note: Runs until the lifecycle stops and blocks on the lifecycle events while idle.
    class AIThread
    {
    private:
        using steady_clock = std::chrono::steady_clock;

        using BridgeClock = Platform::Bridge::Type::BridgeClock;

        using CameraStore = Viewer::Camera::State::CameraStore;

        using TickTraceUIService = UI::TickTrace::System::TickTraceUIService;

    public:
        AIThread(Service::Layer& service, Platform::Layer& platform, Map::Layer& map,
            Resolved::Layer& resolved, Datum::Layer& datumLayer, Tables::Layer& tables, Relations::Layer& relations,
            Environment::Layer& environment, Egocentric::Layer& egocentric,
            Export::Layer& exportLayer, CameraStore& cameraStore, TickTraceUIService& tickTraceUIService) :
            m_Service(service), m_Platform(platform), m_Map(map),
            m_Resolved(resolved), m_Datum(datumLayer), m_Tables(tables), m_Relations(relations),
            m_Environment(environment), m_Egocentric(egocentric),
            m_Export(exportLayer), m_ViewerCameraStore(cameraStore), m_TickTraceUIService(tickTraceUIService) {}
        ~AIThread() = default;

        // Runs the thread loop until the lifecycle stops.
        auto Run() -> void;

    private:
        Service::Layer& m_Service;
        Platform::Layer& m_Platform;
        Map::Layer& m_Map;
        Resolved::Layer& m_Resolved;
        Datum::Layer& m_Datum;
        Tables::Layer& m_Tables;
        Relations::Layer& m_Relations;
        Environment::Layer& m_Environment;
        Egocentric::Layer& m_Egocentric;
        Export::Layer& m_Export;
        CameraStore& m_ViewerCameraStore;
        // Development tool (D72): records every tick while the Simulation tab has the tick trace on.
        TickTraceUIService& m_TickTraceUIService;

        // Stages of a sweep, in the order they run. Timed to find the one behind a slow sweep.
        static constexpr std::array<std::string_view, 24> k_StageNames{
            "Input", "Datum", "ObjectTable", "PlayerTable", "InteractionTable", "GameTable", "Classifier", "ObjectGraph",
            "VitalityDetails", "ZoneDetails", "TeleporterDetails", "LiftDetails", "BarrierDetails", "Entities", "Players",
            "Collidable", "Aim", "Events", "Match", "Self", "Affordance", "Raycast", "Assemble", "Other" };

        bool m_IsLoaded{ false };

        std::uint64_t m_Last{ 0 };
        std::uint64_t m_Dropped{ 0 };

        // Clock of the bridge read with the current tick (D135): its action tick and whether a frame goes to Python.
        BridgeClock m_BridgeClock{};

        // Time of each stage in the current sweep, in nanoseconds.
        std::array<std::uint64_t, k_StageNames.size()> m_StageNs{};
        // Next stage to record.
        std::size_t m_Stage{ 0 };
        steady_clock::time_point m_StageStart{};

        // Builds the map, definitions, world and vitality data once the map file is loaded.
        auto LoadResources() -> void;

        // Updates the tables, relations, environment and egocentric services in order.
        auto ExecuteTick() -> void;

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