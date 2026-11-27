export module DevTools.Viewer.Scene.System;

import Common.Math.Type;
import Services.Observed.State;
import Services.Settings.State;
import Engine.Base.Memory.Type;
import Engine.Control.Simulation.Type;
import Engine.Control.Simulation.State;
import Engine.Base.Pov.Type;
import Engine.Base.Pov.State;
import Engine.Control.Input.State;
import Engine.Render.State;
import Views.View.Type;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Camera.State;
import DevTools.Viewer.Camera.System;
import DevTools.Viewer.Collidable.Type;
import DevTools.Viewer.Collidable.State;
import DevTools.Viewer.Director.Type;
import DevTools.Viewer.Director.State;
import DevTools.Viewer.Director.System;
import DevTools.Viewer.Interpolation.Type;
import DevTools.Viewer.Options.Type;
import DevTools.Viewer.Options.State;
import DevTools.Viewer.Palette.System;
import DevTools.Viewer.Roster.Type;
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
        using PossessRequestStore = Engine::Control::Input::State::PossessRequestStore;
        using RenderStore = Engine::Render::State::RenderStore;
        using CameraStore = DevTools::Viewer::Camera::State::CameraStore;
        using CameraService = DevTools::Viewer::Camera::System::CameraService;
        using Viewport = DevTools::Viewer::Camera::Type::Viewport;
        using Vec3 = Common::Math::Type::Vec3;
        using Matrix = Common::Math::Type::Matrix;
        using GameCamera = Engine::Base::Memory::Type::GameCamera::GameCamera;
        using CollidableScene = DevTools::Viewer::Collidable::Type::CollidableScene;
        using CollidableStore = DevTools::Viewer::Collidable::State::CollidableStore;
        using DirectorFrame = DevTools::Viewer::Director::Type::DirectorFrame;
        using DirectorStore = DevTools::Viewer::Director::State::DirectorStore;
        using DirectorService = DevTools::Viewer::Director::System::DirectorService;
        using FramePoses = DevTools::Viewer::Interpolation::Type::FramePoses;
        using ScenePasses = DevTools::Viewer::Options::Type::ScenePasses;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;
        using Bot = Engine::Control::Simulation::Type::Bot;
        using BotStore = Engine::Control::Simulation::State::BotStore;
        using RosterEntry = DevTools::Viewer::Roster::Type::RosterEntry;
        using SelectionService = DevTools::Viewer::Selection::System::SelectionService;
        using PaletteService = DevTools::Viewer::Palette::System::PaletteService;

    public:
        SceneService(SettingsStore& settingsStore, ObservedPlayerStore& observedStore, CameraStore& cameraStore,
            CameraService& cameraService, SelectionService& selectionService, const RenderStore& renderStore, PovRequestStore& povRequestStore,
            PossessRequestStore& possessRequestStore, const BotStore& botStore, const OptionsStore& optionsStore, DirectorStore& directorStore,
            DirectorService& directorService) :
            m_SettingsStore(settingsStore), m_ObservedStore(observedStore), m_CameraStore(cameraStore),
            m_CameraService(cameraService), m_SelectionService(selectionService), m_RenderStore(renderStore), m_PovRequestStore(povRequestStore),
            m_PossessRequestStore(possessRequestStore), m_BotStore(botStore), m_OptionsStore(optionsStore), m_DirectorStore(directorStore),
            m_DirectorService(directorService) {}
        ~SceneService() = default;

        SceneService(const SceneService&) = delete;
        auto operator=(const SceneService&) -> SceneService& = delete;

        // return: True if the free camera is active.
        auto IsActive() const -> bool;

        // Makes the camera place itself on the local player the next time it is activated, and withdraws the POV request so the game view goes
        // back to the local player (VW.9a). It also stops the director (VW.14).
        auto Deactivate() -> void;

        // Updates the camera, rebuilds the palette and resolves the selection for this frame.
        // note: Builds the roster of the frame, runs the director (VW.14: the pose it gives goes to the camera before the camera updates, and the player of
        // a POV shot becomes the selection), applies the arrow steps asked since the last frame (VW.9b) and then follows the selected player.
        // note: The observed player of the Services (the one the AI thread gives a View) is the player of the selected biped, else the local player.
        // param scene: The scene acquired once for the frame (D294). It is kept as the scene of the last update.
        auto Update(const std::shared_ptr<const CollidableScene>& scene, const Viewport& viewport) -> void;

        // return: Whether a scene update with a pose to draw ran since the last EndFrame, so the labels of the Present frame can be drawn. The
        // update runs at the scene point, so a frame without one (a menu, a pause without a draw) has no labels.
        auto HasFrameUpdate() const -> bool;

        // Lets the director take the camera in a frame that has no scene yet, as long as it plays: the match just loaded and the first Tick is not
        // published. A Fixed or Orbit shot gives its pose to the override at once, so the game does not show the local player first. Nothing is drawn.
        auto PreRoll(const Viewport& viewport) -> void;

        // Closes the frame: the next one has no update until the scene point runs. Called at Present after the labels.
        auto EndFrame() -> void;

        // return: The scene of the last update, so the labels and panels of a frame see the same tick as the 3D passes drawn earlier in it. While
        // the viewer is inactive, or the map was reset, a fresh acquire.
        auto GetFrameScene(const CollidableStore& store) const -> std::shared_ptr<const CollidableScene>;

        // return: The View of the player the viewer watches (the observed one, else the primary local player), or null when the tick has none.
        // note: Valid as long as the tick is.
        auto GetObservedView(const Tick& tick) const -> const View*;

        // return: The rows of Players (and of the ViewTable) that the View layer draws in this frame (VW.17a, D385): the player in focus, else every agent, else the
        // local player. The focus is the selected player, which is also the one the POV watches (Watch POV and the POV shots follow the selection).
        // note: Computed once per update. Empty without a tick.
        auto GetShownRows() const -> const std::vector<std::size_t>&;

        // return: The rows that the observation layer draws in this frame: the shown rows that are agents. The observation is the one of the policy of an agent,
        // so a selected human (it has a side) draws no arrows.
        auto GetObsRows() const -> const std::vector<std::size_t>&;

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
        // shows its death screen for that player then (VW.9a B7). Only the telemetry reads it, the HUD is not hidden (D346).
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

        // return: The 3D passes the scene point runs in a frame, from the master switch of the debug overlay and the level of the observation layer.
        // The telemetry says which one runs, or that none does.
        auto GetScenePasses() const -> ScenePasses;

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

        // return: Every player of the table of the frame scene, ordered by team, humans by name and then bots by number (VW.9b). Built once per update,
        // empty without a tick. The arrows walk it and the strip draws from it.
        auto GetRoster() const -> const std::vector<RosterEntry>&;

        // return: True if the player with that handle is an Artemis bot of the roster (a human, or a player not in the table, is not).
        auto IsBot(std::uint32_t handle) const -> bool;

    private:
        std::unordered_set<std::uint32_t> m_Hidden{};
        std::shared_ptr<const CollidableScene> m_Scene{};
        FramePoses m_FramePoses{};
        bool m_IsFrameUpdated{ false };
        std::vector<RosterEntry> m_Roster{};
        std::vector<Bot> m_Bots{};
        std::vector<std::size_t> m_ShownRows{};
        std::vector<std::size_t> m_ObsRows{};

        // The player the selection follows because a POV shot of the director put it there, k_NoHandle when the director did not.
        std::uint32_t m_DirectedPlayer{ 0xFFFFFFFF };

        // return: The roster of the players of the tick, sorted.
        auto BuildRoster(const Tick& tick) -> std::vector<RosterEntry>;

        // Moves the selection to the player delta positions away in the roster, from the selected player or, with none, to the first or last.
        auto StepSelection(int delta, const std::shared_ptr<const Tick>& tick) -> void;

        // Resolves the selection to a player (VW.9a B6) and rewrites the selected object as the current biped of that player.
        // return: The player the selection follows, or 0xFFFFFFFF: the selection is not a player, or the tick has no players (the follow is kept).
        auto FollowSelectedPlayer(const std::shared_ptr<const Tick>& tick) -> std::uint32_t;

        // Resolves the focus (the observed player) to its row and builds the rows of GetShownRows and GetObsRows for the tick.
        auto ResolveShownRows(const std::shared_ptr<const Tick>& tick, std::uint32_t focusHandle) -> void;

        // Builds the hidden set of GetHiddenHandles.
        auto HiddenHandlesOf(const Tick& tick) const -> std::unordered_set<std::uint32_t>;

        // Runs the director for this frame and applies what it asks to the camera: the free camera with the pose of a fixed or orbit shot, the game camera for
        // a POV shot.
        // return: The frame of the director.
        auto RunDirector(const std::shared_ptr<const Tick>& tick) -> DirectorFrame;

        // Makes the selection follow the player of a POV shot, so the card and the strip show who is seen.
        auto FollowDirectedPlayer(std::uint32_t playerHandle) -> void;

        // Clears the selection when the director set it and its POV shot is over. A selection the user made is not touched.
        auto ReleaseDirectedSelection() -> void;

        // Answers the requests to add a fixed or an orbit shot from the free camera.
        auto AddDirectorShots() -> void;

        SettingsStore& m_SettingsStore;
        ObservedPlayerStore& m_ObservedStore;
        CameraStore& m_CameraStore;
        CameraService& m_CameraService;
        SelectionService& m_SelectionService;
        const RenderStore& m_RenderStore;
        PovRequestStore& m_PovRequestStore;
        PossessRequestStore& m_PossessRequestStore;
        const BotStore& m_BotStore;
        const OptionsStore& m_OptionsStore;
        DirectorStore& m_DirectorStore;
        DirectorService& m_DirectorService;

        PaletteService m_PaletteService{};
    };
}