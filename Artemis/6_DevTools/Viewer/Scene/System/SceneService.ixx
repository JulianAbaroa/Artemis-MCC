export module DevTools.Viewer.Scene.System;

import Common.Math.Type;
import Services.Observed.State;
import Services.Settings.State;
import Engine.Base.Memory.Type;
import Engine.Base.Pov.Type;
import Engine.Base.Pov.State;
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
        using PovRequestStore = Engine::Base::Pov::State::PovRequestStore;
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
            CameraService& cameraService, SelectionService& selectionService, const RenderStore& renderStore, PovRequestStore& povRequestStore) :
            m_SettingsStore(settingsStore), m_ObservedStore(observedStore), m_CameraStore(cameraStore),
            m_CameraService(cameraService), m_SelectionService(selectionService), m_RenderStore(renderStore), m_PovRequestStore(povRequestStore) {}
        ~SceneService() = default;

        SceneService(const SceneService&) = delete;
        auto operator=(const SceneService&) -> SceneService& = delete;

        // return: True if the free camera is active.
        auto IsActive() const -> bool;

        // Makes the camera place itself on the local player the next time it is activated, and withdraws the POV request so the game view goes
        // back to the local player (VW.9a).
        auto Deactivate() -> void;

        // Updates the camera, rebuilds the palette and resolves the selection for this frame.
        // note: The observed player of the Services (the one the AI thread gives a View) is the player of the selected biped, else the local player.
        // param scene: The scene acquired once for the frame (D294). It is kept as the scene of the last update.
        auto Update(const std::shared_ptr<const CollidableScene>& scene, const Viewport& viewport) -> void;

        // return: Whether a scene update with a pose to draw ran since the last EndFrame, so the labels of the Present frame can be drawn. The
        // update runs at the scene point, so a frame without one (a menu, a pause without a draw) has no labels.
        auto HasFrameUpdate() const -> bool;

        // Closes the frame: the next one has no update until the scene point runs. Called at Present after the labels.
        auto EndFrame() -> void;

        // return: The scene of the last update, so the labels and panels of a frame see the same tick as the 3D passes drawn earlier in it. While
        // the viewer is inactive, or the map was reset, a fresh acquire.
        auto GetFrameScene(const CollidableStore& store) const -> std::shared_ptr<const CollidableScene>;

        // return: The View of the player the viewer watches (the observed one, else the primary local player), or null when the tick has none.
        // note: Valid as long as the tick is.
        auto GetObservedView(const Tick& tick) const -> const View*;

        // return: The handles of the unit the game hid in first person and of the objects hanging from it (weapons in hand), when the camera is in its eyes.
        // Empty with the free camera, without the unit hook, or when the game hid nothing.
        // note: Computed once per frame by Update. In Game mode it is the object the first person test of the game answered true for user 0 in the last
        // closed frame (RenderStore::GetFirstPersonUnitObjectLastFrame, one frame behind): the local player, or the watched player with the POV. In a
        // vehicle or turret nothing is hidden when the game draws the body. Without the unit hook nothing is hidden (the telemetry shows whether it is
        // installed). Drawn, that biped and its aim spheres cover the scene.
        auto GetHiddenHandles() const -> const std::unordered_set<std::uint32_t>&;

        // return: The distance from the game camera to the eye of the View of the local player, or nothing without a valid game camera or a View body.
        auto GetEyeDistance(const Tick& tick) const -> std::optional<float>;

        // return: Whether the game view shows the requested player (status) and that player has no live body in the scene of the frame: the game
        // repeats its loadout screen then, and the Viewer hides the game HUD (VW.9a B7).
        auto IsPovPlayerDead(const Engine::Base::Pov::Type::PovStatus& status) const -> bool;

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

        // return: True if the camera mode is Game (the free camera is the other mode).
        auto IsGameMode() const -> bool;

        auto ClearSelection() -> void;

        // Drops the palette and the selection boxes.
        // note: MapService calls it when it releases the map.
        auto Reset() -> void;

        auto GetViewProjection() const -> const Matrix&;

        // return: The camera of the game this frame, or nothing when that camera is not valid.
        auto GetGameCamera() const -> const std::optional<GameCamera>&;

        // return: Whether the override of the game camera is published.
        auto IsOverrideActive() const -> bool;

        // return: How many faults the observer hook caught while it touched the engine memory.
        auto GetOverrideFaults() const -> std::uint32_t;

        // return: The selected handle, or k_NoHandle.
        auto GetSelected() const -> std::uint32_t;
        auto GetPalette() const -> const PaletteService&;

    private:
        std::unordered_set<std::uint32_t> m_Hidden{};
        std::shared_ptr<const CollidableScene> m_Scene{};
        FramePoses m_FramePoses{};
        bool m_IsFrameUpdated{ false };

        // Resolves the selection to a player (VW.9a B6) and rewrites the selected object as the current biped of that player.
        // return: The player the selection follows, or 0xFFFFFFFF: the selection is not a player, or the tick has no players (the follow is kept).
        auto FollowSelectedPlayer(const std::shared_ptr<const Tick>& tick) -> std::uint32_t;

        // Builds the hidden set of GetHiddenHandles.
        auto HiddenHandlesOf(const Tick& tick) const -> std::unordered_set<std::uint32_t>;

        SettingsStore& m_SettingsStore;
        ObservedPlayerStore& m_ObservedStore;
        CameraStore& m_CameraStore;
        CameraService& m_CameraService;
        SelectionService& m_SelectionService;
        const RenderStore& m_RenderStore;
        PovRequestStore& m_PovRequestStore;

        PaletteService m_PaletteService{};
    };
}