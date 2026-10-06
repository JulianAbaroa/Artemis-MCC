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
import Runtime.Thread;
import std;

export namespace Runtime
{
    // Root of the Runtime layer. Owns the main, AI and input threads.
    // note: Run() starts the AI and input threads and then runs the main thread on the caller.
    class Layer
    {
    private:
        using CameraStore = Viewer::Camera::State::CameraStore;

        using MainThread = Runtime::Thread::MainThread;
        using AIThread = Runtime::Thread::AIThread;
        using InputThread = Runtime::Thread::InputThread;

    public:
        Layer(Service::Layer& service, Platform::Layer& platform, Map::Layer& map,
            Resolved::Layer& resolved, Tables::Layer& tables,
            Relations::Layer& relations, Environment::Layer& environment,
            Egocentric::Layer& egocentric, Export::Layer& exportLayer,
            CameraStore& viewerCameraStore) :
            m_Main(service, platform),
            m_AI(service, platform, map, resolved, tables, relations,
                environment, egocentric, exportLayer, viewerCameraStore),
            m_Input(service, platform) {}
        ~Layer() = default;

        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // Starts the AI and input threads and runs the main thread until the lifecycle stops.
        auto Run() -> void
        {
            m_AIThread = std::jthread([this] { m_AI.Run(); });
            m_InputThread = std::jthread([this] { m_Input.Run(); });

            m_Main.Run();
        }

    private:
        MainThread m_Main;
        AIThread m_AI;
        InputThread m_Input;

        std::jthread m_AIThread;
        std::jthread m_InputThread;
    };
}