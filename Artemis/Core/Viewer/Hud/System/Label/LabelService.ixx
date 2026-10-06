export module Viewer.Hud.System:Labels;

import :Canvas;
import :PlayerLabels;
import :ObjectLabels;
import :VehicleLabels;
import :HealthLabels;
import :InteractionLabels;
import :FixtureLabels;
import Relations.Classifier.Type;
import Export.Tick.State;
import Viewer.Camera.System;
import Viewer.Scene.System;
import Viewer.Options.State;
import Viewer.Selection.State;
import Viewer.Hud.Type;
import std;

export namespace Viewer::Hud::System
{
    // Builds the labels of every collector each frame and draws them on the background of the screen.
    class LabelService
    {
    private:
        using Role = Relations::Classifier::Type::Role;
        using LabelContext = Viewer::Hud::Type::LabelContext;

        using TickStore = Export::Tick::State::TickStore;
        using CameraService = Viewer::Camera::System::CameraService;
        using SceneService = Viewer::Scene::System::SceneService;
        using OptionsStore = Viewer::Options::State::OptionsStore;
        using SelectionStore = Viewer::Selection::State::SelectionStore;

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
        std::unordered_map<std::uint32_t, Role> m_Roles{};
    };
}