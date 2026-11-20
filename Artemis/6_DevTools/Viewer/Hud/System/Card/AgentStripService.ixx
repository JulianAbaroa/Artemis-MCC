export module DevTools.Viewer.Hud.System:AgentStrip;

import DevTools.Viewer.Scene.System;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Selection.State;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Draws the strip with the previous, current and next player of the roster at the top center of the game frame (VW.9b). The left and right arrows
    // step along that roster.
    class AgentStripService
    {
    private:
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;

    public:
        AgentStripService(SceneService& sceneService, OptionsStore& optionsStore, SelectionStore& selectionStore) :
            m_SceneService(sceneService), m_OptionsStore(optionsStore), m_SelectionStore(selectionStore) {}
        ~AgentStripService() = default;

        AgentStripService(const AgentStripService&) = delete;
        auto operator=(const AgentStripService&) -> AgentStripService& = delete;

        // Draws the strip of the selected player, with the roster of the frame scene.
        // note: Does nothing without a selected player that is in the roster. The caller gives the same gate as the card (active Viewer and a scene
        // updated in this frame).
        auto Draw() -> void;

    private:
        SceneService& m_SceneService;
        OptionsStore& m_OptionsStore;
        SelectionStore& m_SelectionStore;
    };
}