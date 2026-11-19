export module DevTools.Viewer.Scene.System;

import Common.Math.Type;
import Services.Observed.State;
import Services.Settings.State;
import Engine.Base.Memory.Type;
import Engine.Render.State;
import Views.View.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Camera.State;
import DevTools.Viewer.Camera.System;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Interpolation.Type;
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
        using RenderStore = Engine::Render::State::RenderStore;
        using CameraStore = DevTools::Viewer::Camera::State::CameraStore;
        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using Viewport = DevTools::Viewer::Camera::Type::Viewport;
        using Vec3 = Common::Math::Type::Vec3;
        using Matrix = Common::Math::Type::Matrix;
        using GameCamera = Engine::Base::Memory::Type::GameCamera::GameCamera;
        using CollidableScene = DevTools::Viewer::Collidable::Type::CollidableScene;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using FramePoses = DevTools::Viewer::Interpolation::Type::FramePoses;
        using SelectionService = DevTools::Viewer::Selection::System::SelectionService;
        using PaletteService = DevTools::Viewer::Palette::System::PaletteService;

    public:
        SceneService(SettingsStore& settingsStore, ObservedPlayerStore& observedStore, CameraStore& cameraStore,
            CameraService& cameraService, SelectionService& selectionService, const RenderStore& renderStore) :
            m_SettingsStore(settingsStore), m_ObservedStore(observedStore), m_CameraStore(cameraStore),
            m_CameraService(cameraService), m_SelectionService(selectionService), m_RenderStore(renderStore) {}
        ~SceneService() = default;

        SceneService(const SceneService&) = delete;
        auto operator=(const SceneService&) -> SceneService& = delete;

        // return: Whether the biped of the local player is hidden in Game mode (VW.6b.1).
        // param gameHidesUnit: Whether the game hid the unit of user 0 in the last closed frame, nothing without the unit hook.
        // param eyeDistance: Distance from the game camera to the View eye, nothing without a valid game camera.
        // note: The answer of the game wins. Without it the camera counts as in the eyes when closer than k_FirstPersonDistance, and without a
        // valid camera the camera sits on the View, so it is hidden.
        static auto IsOwnBipedHidden(std::optional<bool> gameHidesUnit, std::optional<float> eyeDistance) -> bool;

        // return: True if the free camera is active.
        auto IsActive() const -> bool;

        // Makes the camera place itself on the local player the next time it is activated.
        auto Deactivate() -> void;

        // Updates the camera, rebuilds the palette and resolves the selection for this frame.
        // note: Follows or orbits the selected object according to the follow mode of the camera. When it follows the biped of a player, that
        // player is the observed one of the Services (the AI thread gives it a View).
        // param scene: The scene acquired once for the frame (D294). It is kept as the scene of the last update.
        auto Update(const std::shared_ptr<const CollidableScene>& scene, const Viewport& viewport) -> void;

        // return: The scene of the last update, so the labels and panels of a frame see the same tick as the 3D passes drawn earlier in it
        // (they may run before the HUD of the game and at Present). While the viewer is inactive, or the map was reset, a fresh acquire.
        auto GetFrameScene(const CollidableStore& store) const -> std::shared_ptr<const CollidableScene>;

        // return: The View of the player the viewer watches (the observed one, else the primary local player), or null when the tick has none.
        // note: Valid as long as the tick is.
        auto GetObservedView(const Tick& tick) const -> const View*;

        // return: The handles of the biped of the player whose eyes the camera is in, and of the objects hanging from it (weapons in hand).
        // Empty with the Orbit follow, the free camera that follows nothing, or a View without a biped.
        // note: Computed once per frame by Update. In Game mode it is the primary local player when the game hid its unit in the last closed
        // frame (RenderStore::WasFirstPersonUnitLastFrame, one frame behind), so with Drive nothing is hidden (in a vehicle or turret expected too, not measured). Without
        // the unit hook the camera must be within k_FirstPersonDistance of the View eye (on foot). With the FirstPerson
        // follow it is the followed player. Drawn, that biped and its aim spheres cover the scene.
        auto GetHiddenHandles() const -> const std::unordered_set<std::uint32_t>&;

        // return: The distance from the game camera to the eye of the View of the local player, or nothing without a valid game camera or a View body.
        auto GetEyeDistance(const Tick& tick) const -> std::optional<float>;

        // return: The eye of the View of the local player in the tick, or nothing without a View body.
        auto GetLocalEye(const Tick& tick) const -> std::optional<Vec3>;

        // return: The world position of the vehicle the local player is in (the parent of its biped in the object graph), or nothing on foot or
        // without a View body.
        auto GetLocalVehiclePosition(const Tick& tick) const -> std::optional<Vec3>;

        // return: The handle of the vehicle the local player is in, or the no-handle value on foot or without a View.
        auto GetLocalVehicleHandle(const Tick& tick) const -> std::uint32_t;

        // return: The eye of the local player with the pose the overlay draws its biped in (VW.5e), the plain eye when the biped is not blended.
        auto GetLocalEyeRender(const Tick& tick) const -> std::optional<Vec3>;

        // return: The position of the vehicle of the local player with the pose the overlay draws it in (VW.5e).
        auto GetLocalVehicleRender(const Tick& tick) const -> std::optional<Vec3>;

        // Keeps the poses the objects are drawn with in this frame (VW.5e, D330), for what is drawn apart from the passes: the labels.
        // note: Set once per frame by MapService after Update, empty when nothing is blended. Read on the thread that draws.
        auto SetFramePoses(const FramePoses& poses) -> void;

        auto GetFramePoses() const -> const FramePoses&;

        // return: True if the camera mode is Game, the free camera driving the game camera not included (IsGameCamera counts both).
        auto IsGameMode() const -> bool;

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
        std::shared_ptr<const CollidableScene> m_Scene{};
        FramePoses m_FramePoses{};

        // Builds the hidden set of GetHiddenHandles.
        auto HiddenHandlesOf(const Tick& tick) const -> std::unordered_set<std::uint32_t>;

        SettingsStore& m_SettingsStore;
        ObservedPlayerStore& m_ObservedStore;
        CameraStore& m_CameraStore;
        CameraService& m_CameraService;
        SelectionService& m_SelectionService;
        const RenderStore& m_RenderStore;

        PaletteService m_PaletteService{};
    };
}