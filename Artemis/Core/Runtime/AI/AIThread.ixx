export module Runtime.Thread:AI;

import Service.Layer;
import Platform.Layer;
import Map.Layer;
import Resolved.Layer;
import Template.Layer;
import Tables.Layer;
import Relations.Layer;
import Environment.Layer;
import Egocentric.Layer;
import Export.Layer;
import Viewer.Camera.State;
import std;

export namespace Runtime::Thread
{
    // Thread that loads the map resources and runs the simulation tick pipeline.
    // note: Runs until the lifecycle stops and blocks on the lifecycle events while idle.
    class AIThread
    {
    private:
        using CameraStore = Viewer::Camera::State::CameraStore;

    public:
        AIThread(Service::Layer& service, Platform::Layer& platform, Map::Layer& map,
            Resolved::Layer& resolved, Template::Layer& templateLayer, Tables::Layer& tables, Relations::Layer& relations,
            Environment::Layer& environment, Egocentric::Layer& egocentric,
            Export::Layer& exportLayer, CameraStore& cameraStore) :
            m_Service(service), m_Platform(platform), m_Map(map),
            m_Resolved(resolved), m_Template(templateLayer), m_Tables(tables), m_Relations(relations),
            m_Environment(environment), m_Egocentric(egocentric),
            m_Export(exportLayer), m_ViewerCameraStore(cameraStore) {}
        ~AIThread() = default;

        // Runs the thread loop until the lifecycle stops.
        auto Run() -> void;

    private:
        Service::Layer& m_Service;
        Platform::Layer& m_Platform;
        Map::Layer& m_Map;
        Resolved::Layer& m_Resolved;
        Template::Layer& m_Template;
        Tables::Layer& m_Tables;
        Relations::Layer& m_Relations;
        Environment::Layer& m_Environment;
        Egocentric::Layer& m_Egocentric;
        Export::Layer& m_Export;
        CameraStore& m_ViewerCameraStore;

        bool m_IsLoaded{ false };

        std::uint64_t m_Last{ 0 };
        std::uint64_t m_Dropped{ 0 };

        // Builds the map, definitions, world and vitality data once the map file is loaded.
        auto LoadResources() -> void;

        // Updates the tables, relations, environment and egocentric services in order.
        auto ExecuteTick() -> void;

        // Checks whether the lifecycle is running and not tearing down.
        auto IsStable() -> bool;

        // Clears the tick generation and the load state for the next map.
        auto Reset() -> void;
    };
}