export module DevTools.Viewer.Hud.System:Labels;

import :Canvas;
import :PlayerLabels;
import :ObjectLabels;
import :VehicleLabels;
import :HealthLabels;
import :InteractionLabels;
import :FixtureLabels;

import Perception.Classifier.Type;
import DevTools.Viewer.Camera.System;
import DevTools.Viewer.Collidable.State;
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

        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using SceneService = DevTools::Viewer::Scene::System::SceneService;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using SelectionStore = DevTools::Viewer::Selection::State::SelectionStore;

    public:
        // param collidableStore: Source of the Tick drawn, the one of the collidable scene (D294).
        LabelService(CollidableStore& collidableStore, CameraService& cameraService,
            SceneService& sceneService, OptionsStore& optionsStore,
            SelectionStore& selectionStore) :
            m_CollidableStore(collidableStore), m_SceneService(sceneService),
            m_OptionsStore(optionsStore), m_SelectionStore(selectionStore),
            m_Canvas(cameraService, optionsStore) {}
        ~LabelService() = default;

        LabelService(const LabelService&) = delete;
        auto operator=(const LabelService&) -> LabelService& = delete;

        // Draws the labels of the current tick.
        // note: Does nothing if the scene is inactive or the labels are disabled.
        auto Draw() -> void;

    private:
        CollidableStore& m_CollidableStore;
        SceneService& m_SceneService;
        OptionsStore& m_OptionsStore;
        SelectionStore& m_SelectionStore;

        LabelCanvas m_Canvas;
        std::unordered_map<std::uint32_t, const Classified*> m_Classifications{};
    };
}