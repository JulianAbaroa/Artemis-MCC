export module DevTools.Viewer.Scene.System;

import Common.Math.Type;
import Services.Observed.State;
import Services.Settings.State;
import Engine.Base.Memory.Type;
import Views.View.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Camera.State;
import DevTools.Viewer.Camera.System;
import DevTools.Viewer.Palette.System;
import DevTools.Viewer.Selection.System;
import std;

export namespace DevTools::Viewer::Scene::System
{
    // Prepares what the map and the HUD draw each frame: camera, color palette and selection.
    // It is the single entry point for the viewer systems that need the scene.
    class SceneService
    {
    private:
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using View = Views::View::Type::View;
        using ObservedPlayerStore = Services::Observed::State::ObservedPlayerStore;
        using SettingsStore = Services::Settings::State::SettingsStore;
        using CameraStore = DevTools::Viewer::Camera::State::CameraStore;
        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using Viewport = DevTools::Viewer::Camera::Type::Viewport;
        using Matrix = Common::Math::Type::Matrix;
        using GameCamera = Engine::Base::Memory::Type::GameCamera::GameCamera;
        using SelectionService = DevTools::Viewer::Selection::System::SelectionService;
        using PaletteService = DevTools::Viewer::Palette::System::PaletteService;

    public:
        SceneService(SettingsStore& settingsStore, ObservedPlayerStore& observedStore, CameraStore& cameraStore,
            CameraService& cameraService, SelectionService& selectionService) :
            m_SettingsStore(settingsStore), m_ObservedStore(observedStore), m_CameraStore(cameraStore),
            m_CameraService(cameraService), m_SelectionService(selectionService) {}
        ~SceneService() = default;

        SceneService(const SceneService&) = delete;
        auto operator=(const SceneService&) -> SceneService& = delete;

        // return: True if the free camera is active.
        auto IsActive() const -> bool;

        // Makes the camera place itself on the local player the next time it is activated.
        auto Deactivate() -> void;

        // Updates the camera, rebuilds the palette and resolves the selection for this frame.
        // note: Follows or orbits the selected object according to the follow mode of the camera. When it follows the biped of a player, that
        // player is the observed one of the Services (the AI thread gives it a View).
        auto Update(const std::shared_ptr<const Tick>& tick, const Viewport& viewport) -> void;

        // return: The View of the player the viewer watches (the observed one, else the primary local player), or null when the tick has none.
        // note: Valid as long as the tick is.
        auto GetObservedView(const Tick& tick) const -> const View*;

        // return: The handles of the biped of the player whose eyes the camera is in, and of the objects hanging from it (weapons in hand).
        // Empty with the Orbit follow, the free camera that follows nothing, or a View without a biped.
        // note: Computed once per frame by Update. In Game mode the camera is in the eyes of the primary local player only when the game
        // camera is within k_FirstPersonDistance of the View eye (on foot), so in a vehicle or turret nothing is hidden. With the FirstPerson
        // follow it is the followed player. Drawn, that biped and its aim spheres cover the scene.
        auto GetHiddenHandles() const -> const std::unordered_set<std::uint32_t>&;

        // return: The distance from the game camera to the eye of the View of the local player, or nothing without a valid game camera or a View body.
        auto GetEyeDistance(const Tick& tick) const -> std::optional<float>;

        auto ClearSelection() -> void;

        // Drops the palette and the selection boxes.
        // note: MapService calls it when it releases the map.
        auto Reset() -> void;

        auto GetViewProjection() const -> const Matrix&;

        // return: True if the camera mode is Game (the camera of the game, or the View of the local player when the game one is not valid), or the
        // free camera drives the game camera (then the frame under the Viewer is the one of the game camera).
        auto IsGameCamera() const -> bool;

        // return: The camera of the game this frame, or nothing when the mode is not Game, the free camera is not driving it or that camera is
        // not valid.
        auto GetGameCamera() const -> const std::optional<GameCamera>&;

        // return: Whether the override of the game camera is published.
        auto IsOverrideActive() const -> bool;

        // return: How many faults the observer hook caught while it touched the engine memory.
        auto GetOverrideFaults() const -> std::uint32_t;

        // return: The distance between the eye asked of the game and the position of its camera, or nothing when the camera is not driving.
        auto GetOverrideDistance() const -> std::optional<float>;

        // return: The selected handle, or k_NoHandle.
        auto GetSelected() const -> std::uint32_t;
        auto GetPalette() const -> const PaletteService&;

    private:
        std::unordered_set<std::uint32_t> m_Hidden{};

        // Builds the hidden set of GetHiddenHandles.
        auto HiddenHandlesOf(const Tick& tick) const -> std::unordered_set<std::uint32_t>;

        SettingsStore& m_SettingsStore;
        ObservedPlayerStore& m_ObservedStore;
        CameraStore& m_CameraStore;
        CameraService& m_CameraService;
        SelectionService& m_SelectionService;

        PaletteService m_PaletteService{};
    };
}