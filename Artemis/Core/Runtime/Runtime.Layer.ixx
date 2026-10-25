export module Runtime.Layer;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Resolved.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import Viewer.Camera.State;
import UI.Layer;
import Runtime.Thread;
import std;

export namespace Runtime
{
    // Root of the Runtime layer. Owns the main, AI, input, bench, bridge and dump threads.
    // note: Run() starts the AI, input, bench, bridge and dump threads and then runs the main thread on the caller.
    class Layer
    {
    private:
        using CameraStore = Viewer::Camera::State::CameraStore;

        using MainThread = Runtime::Thread::MainThread;
        using AIThread = Runtime::Thread::AIThread;
        using InputThread = Runtime::Thread::InputThread;
        using BenchThread = Runtime::Thread::BenchThread;
        using BridgeThread = Runtime::Thread::BridgeThread;
        using DumpThread = Runtime::Thread::DumpThread;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map,
            Resolved::Layer& resolved, Tables::Layer& tables,
            Relations::Layer& relations, Environment::Layer& environment,
            Egocentric::Layer& egocentric, Export::Layer& exportLayer,
            CameraStore& viewerCameraStore, UI::Layer& ui) :
            m_Main(service, platform),
            m_AI(service, platform, map, resolved, tables, relations,
                environment, egocentric, exportLayer, viewerCameraStore, ui.m_TickTraceUIService, ui.m_TickDumpUIService),
            m_Input(service, platform),
            m_Bench(service, platform, ui.m_BenchUIService),
            m_Bridge(service, platform, exportLayer),
            m_Dump(service, platform, ui.m_TickDumpUIService) {}
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // Starts the AI, input, bench, bridge and dump threads and runs the main thread until the lifecycle stops.
        auto Run() -> void
        {
            m_AIThread = std::jthread([this] { m_AI.Run(); });
            m_InputThread = std::jthread([this] { m_Input.Run(); });
            m_BenchThread = std::jthread([this] { m_Bench.Run(); });
            m_BridgeThread = std::jthread([this] { m_Bridge.Run(); });
            m_DumpThread = std::jthread([this] { m_Dump.Run(); });

            m_Main.Run();
        }

    private:
        MainThread m_Main;
        AIThread m_AI;
        InputThread m_Input;
        BenchThread m_Bench;
        BridgeThread m_Bridge;
        DumpThread m_Dump;

        std::jthread m_AIThread{};
        std::jthread m_InputThread{};
        std::jthread m_BenchThread{};
        std::jthread m_BridgeThread{};
        std::jthread m_DumpThread{};
    };
}