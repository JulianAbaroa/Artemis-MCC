export module Viewer.Scene.System;

import Service.Settings.State;
import Export.Tick.Type;
import Viewer.Camera.State;
import Viewer.Camera.System;
import Viewer.Camera.Type;
import Viewer.Palette.System;
import Viewer.Selection.System;
import std;

export namespace Viewer::Scene::System
{
    // Prepares what the map and the HUD draw each frame: camera, color palette and selection.
    // It is the single entry point for the viewer systems that need the scene.
    class SceneService
    {
    private:
        using Tick = Export::Tick::Type::Tick;
        using SettingsStore = Service::Settings::State::SettingsStore;
        using CameraStore = Viewer::Camera::State::CameraStore;
        using CameraService = Viewer::Camera::System::CameraService;
        using Viewport = Viewer::Camera::Type::Viewport;
        using Matrix = Viewer::Camera::Type::Matrix;
        using SelectionService = Viewer::Selection::System::SelectionService;
        using PaletteService = Viewer::Palette::System::PaletteService;

    public:
        SceneService(SettingsStore& settingsStore, CameraStore& cameraStore,
            CameraService& cameraService, SelectionService& selectionService) :
            m_SettingsStore(settingsStore), m_CameraStore(cameraStore),
            m_CameraService(cameraService), m_SelectionService(selectionService) {}
        ~SceneService() = default;

        SceneService(const SceneService&) = delete;
        auto operator=(const SceneService&) -> SceneService& = delete;

        // return: True if the free camera is active.
        auto IsActive() const -> bool;

        // Makes the camera place itself on the local player the next time it is activated.
        auto Deactivate() -> void;

        // Updates the camera, rebuilds the palette and resolves the selection for this frame.
        // note: Follows the selected object if the follow flag of the camera is set.
        auto Update(const std::shared_ptr<const Tick>& tick, const Viewport& viewport) -> void;

        auto ClearSelection() -> void;

        // Drops the palette and the selection boxes. Call it when the map changes.
        auto Reset() -> void;

        auto GetViewProjection() const -> const Matrix&;

        // return: The selected handle, or k_NoSelection.
        auto GetSelected() const -> std::uint32_t;
        auto GetPalette() const -> const PaletteService&;

    private:
        SettingsStore& m_SettingsStore;
        CameraStore& m_CameraStore;
        CameraService& m_CameraService;
        SelectionService& m_SelectionService;

        PaletteService m_PaletteService{};
    };
}