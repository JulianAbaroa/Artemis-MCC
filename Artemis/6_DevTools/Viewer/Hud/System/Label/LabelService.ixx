export module DevTools.Viewer.Hud.System:Labels;

import :Canvas;
import :PlayerLabels;
import :ObjectLabels;
import :VehicleLabels;
import :HealthLabels;
import :InteractionLabels;
import :FixtureLabels;

import Perception.Classifier.Type;
import Bridge.Codec.Tick.State;
import DevTools.Viewer.Camera.System;
import DevTools.Viewer.Scene.System;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Selection.State;
import DevTools.Viewer.Hud.Type;
import std;

export namespace DevTools::Viewer::Hud::System
{
    // Builds the labels of every collector each frame and draws them on the background of the screen.
    class LabelService
    {
    private:
        using Classified = Perception::Classifier::Type::Classified;
        using LabelContext = DevTools::Viewer::Hud::Type::LabelContext;

        using TickStore = Bridge::Codec::Tick::State::TickStore;
        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;

    public:
        LabelService(TickStore& tickStore, CameraService& cameraService,
            SceneService& sceneService, OptionsStore& optionsStore,
            SelectionStore& selectionStore) :
            m_TickStore(tickStore), m_SceneService(sceneService),
            m_OptionsStore(optionsStore), m_SelectionStore(selectionStore),
            m_Canvas(cameraService, optionsStore) {}
        ~LabelService() = default;

        LabelService(const LabelService&) = delete;
        auto operator=(const LabelService&) -> LabelService& = delete;

        // Draws the labels of the current tick.
        // note: Does nothing if the scene is inactive or the labels are disabled.
        auto Draw() -> void;

    private:
        TickStore& m_TickStore;
        SceneService& m_SceneService;
        OptionsStore& m_OptionsStore;
        SelectionStore& m_SelectionStore;

        LabelCanvas m_Canvas;
        std::unordered_map<std::uint32_t, const Classified*> m_Classifications{};
    };
}