export module DevTools.Layer;

import Service.Layer;
import Service.Preferences.System;
import Map.Layer;
import Resolved.Layer;
import Tables.Layer;
import Environment.Layer;
import Export.Layer;
import Export.Pipeline.Type;
import DevTools.Type;
import Gui.Layer;
import Viewer.Layer;
import UI.Layer;
import std;

export namespace DevTools
{
    // Root of the development tools. Owns the Gui, Viewer and UI layers and the observer that feeds them every tick.
    // note: Layers are built in member order, each one takes references to the ones before it.
    class Layer
    {
    private:
        using PreferencesService = Service::Preferences::System::PreferencesService;

        using EngineAccess = DevTools::Type::EngineAccess;
        using ObservedTick = Export::Pipeline::Type::ObservedTick;

    public:
        Layer(Service::Layer& service, EngineAccess& engine, Map::Layer& map, Resolved::Layer& resolved,
            Tables::Layer& tables, Environment::Layer& environment, Export::Layer& exportLayer,
            PreferencesService& preferences) :
            m_Gui(service, engine.m_RenderService, engine.m_RenderStore),
            m_Viewer(service, engine, environment, exportLayer, m_Gui, preferences),
            m_UI(service, engine, map, resolved, tables, exportLayer, m_Gui, m_Viewer, preferences) {}
        ~Layer() = default;

        // Not copyable. Members hold references to each other.
        Layer(const Layer&) = delete;
        auto operator=(const Layer&) -> Layer& = delete;

        // Gives each tool the tick that just went to the bridge.
        // note: Runs on the AI thread after the push to the bridge (D223). The camera state is read once, so the scene and the recording agree.
        auto OnTick(const ObservedTick& tick) -> void
        {
            const bool isViewerActive{ m_Viewer.m_CameraStore.IsActive() };

            m_Viewer.m_CollidableSceneService.Update(isViewerActive);

            m_UI.m_TickTraceUIService.CaptureClock(*tick.Snapshot);
            m_UI.m_TickTraceUIService.Record(tick.Generation);
            if (m_UI.m_TickDumpUIService.IsRecording())
            {
                m_UI.m_TickDumpUIService.Push(tick.Tick, tick.Snapshot, tick.Generation, tick.DroppedBefore,
                    tick.IsFirstAfterLoad, isViewerActive);
            }
        }

        // The tools, in the order they are built.
        Gui::Layer m_Gui;
        Viewer::Layer m_Viewer;
        UI::Layer m_UI;
    };
}