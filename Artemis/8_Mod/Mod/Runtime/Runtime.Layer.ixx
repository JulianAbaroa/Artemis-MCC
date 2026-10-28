export module Runtime.Layer;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Resolved.Layer;
import Tables.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import Export.Pipeline.Type;
import DevTools.Layer;
import Runtime.Thread;
import std;

export namespace Runtime
{
    // Root of the Runtime layer. Owns the main, AI, bench, bridge and dump threads.
    // note: Run() starts the AI, bench, bridge and dump threads and then runs the main thread on the caller.
    class Layer
    {
    private:
        using ObservedTick = Export::Pipeline::Type::ObservedTick;

        using MainThread = Runtime::Thread::MainThread;
        using AIThread = Runtime::Thread::AIThread;
        using BenchThread = Runtime::Thread::BenchThread;
        using BridgeThread = Runtime::Thread::BridgeThread;
        using DumpThread = Runtime::Thread::DumpThread;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map,
            Resolved::Layer& resolved, Tables::Layer& tables,
            Environment::Layer& environment,
            Egocentric::Layer& egocentric, Export::Layer& exportLayer,
            DevTools::Layer& devTools) :
            m_Main(service, platform.m_Base, platform.m_Match, platform.m_Render),
            m_AI(service, platform.m_Base, platform.m_Capture, platform.m_Transport, map, resolved, tables,
                environment, egocentric, exportLayer),
            m_Bench(service, platform.m_Base, devTools.m_UI.m_BenchUIService),
            m_Bridge(service, platform.m_Base, platform.m_Transport, exportLayer),
            m_Dump(service, platform.m_Base, devTools.m_UI.m_TickDumpUIService)
        {
            m_AI.AddObserver([&devTools](const ObservedTick& tick) {
                devTools.OnTick(tick);
            });
        }
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // Starts the AI, bench, bridge and dump threads and runs the main thread until the lifecycle stops.
        auto Run() -> void
        {
            m_AIThread = std::jthread([this] { m_AI.Run(); });
            m_BenchThread = std::jthread([this] { m_Bench.Run(); });
            m_BridgeThread = std::jthread([this] { m_Bridge.Run(); });
            m_DumpThread = std::jthread([this] { m_Dump.Run(); });

            m_Main.Run();
        }

    private:
        MainThread m_Main;
        AIThread m_AI;
        BenchThread m_Bench;
        BridgeThread m_Bridge;
        DumpThread m_Dump;

        std::jthread m_AIThread{};
        std::jthread m_BenchThread{};
        std::jthread m_BridgeThread{};
        std::jthread m_DumpThread{};
    };
}