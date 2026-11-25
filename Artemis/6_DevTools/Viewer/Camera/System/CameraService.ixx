export module DevTools.Viewer.Camera.System;

import Common.Math.Type;
import Engine.Base.Memory.Type;
import Engine.Base.Memory.System;
import Bridge.Codec.Tick.Type;
import DevTools.Viewer.Camera.Type;
import DevTools.Viewer.Camera.State;
import DevTools.Viewer.Options.State;
import std;

export namespace DevTools::Viewer::Camera::System
{
    // Camera of the viewer: the camera of the game (Game mode) or the free camera, which flies with the keys and the mouse and always moves the camera of the game.
    // It builds the view and projection matrices on every update and projects between the world and the screen.
    class CameraService
    {
    private:
        using steady_clock = std::chrono::steady_clock;
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using Vec2 = Common::Math::Type::Vec2;
        using Vec3 = Common::Math::Type::Vec3;
        using Matrix = Common::Math::Type::Matrix;
        using GameCamera = Engine::Base::Memory::Type::GameCamera::GameCamera;
        using GameCameraService = Engine::Base::Memory::System::GameCameraService;
        using Lens = DevTools::Viewer::Camera::Type::Lens;
        using Viewport = DevTools::Viewer::Camera::Type::Viewport;
        using Ray = DevTools::Viewer::Camera::Type::Ray;
        using CameraStore = DevTools::Viewer::Camera::State::CameraStore;
        using OptionsStore = DevTools::Viewer::Options::State::OptionsStore;

    public:
        CameraService(CameraStore& cameraStore, OptionsStore& optionsStore, GameCameraService& gameCameraService) :
            m_CameraStore(cameraStore), m_OptionsStore(optionsStore), m_GameCameraService(gameCameraService) {}
        ~CameraService() = default;

        CameraService(const CameraService&) = delete;
        auto operator=(const CameraService&) -> CameraService& = delete;

        // Advances the camera and rebuilds the matrices.
        // note: In Game mode the camera is the one the game rendered the frame with (D323), read at the render thread. Without a valid one the
        // camera has no render pose (HasRenderPose) and nothing is drawn. In Free mode the camera flies and asks the game for that pose in every
        // frame (the override of the game camera), and reads the camera the game drew, one frame behind.
        // param acceptInput: If false, the pressed keys are released and the camera does not fly.
        // note: The first update after Deactivate places the camera on the local player.
        auto Update(const std::shared_ptr<const Tick>& tick, const Viewport& viewport, bool acceptInput) -> void;

        // Makes the next Update place the camera on the local player again and gives the game camera back to the game.
        auto Deactivate() -> void;

        // Asks for the pose of the director (VW.14): the next Update in Free mode takes it instead of flying, ignoring the keys and the mouse, and then asks
        // the game for it as every free pose. It is used once. A pose that is not finite is ignored. Yaw and pitch are in radians.
        auto SetScriptedPose(const Vec3& eye, float yaw, float pitch) -> void;

        // The render pose: the one the overlay is drawn with. It is the camera of the game read this frame, or the free pose when the free camera
        // has no valid game camera. With the free camera it is NOT the free pose: the game draws one frame behind the request, so the overlay
        // must use what the game drew (D323). Everything that draws or measures against the frame
        // (view-projection, labels, rays, picking) reads these.
        auto GetEye() const -> Vec3;
        auto GetForward() const -> Vec3;
        auto GetRight() const -> Vec3;
        auto GetUp() const -> Vec3;

        // The requested pose: the free camera (yaw and pitch from the mouse). It is what SetOverride asks of the game and never follows the game
        // camera in Free mode.
        auto GetYaw() const -> float;
        auto GetPitch() const -> float;

        auto GetViewport() const -> const Viewport&;

        // return: The camera of the game this frame, or nothing when the game camera is not valid.
        auto GetGameCamera() const -> const std::optional<GameCamera>&;

        // return: Whether there is a pose to draw with this frame: the camera of the game is valid, or the free camera flies (without a valid game camera it
        // draws with the free pose).
        auto HasRenderPose() const -> bool;

        // return: Whether the override of the game camera is published (it can fail to publish when the pose is not valid).
        auto IsOverrideActive() const -> bool;

        // return: How many faults the observer hook caught while it touched the engine memory.
        auto GetOverrideFaults() const -> std::uint32_t;

        // return: The view matrix multiplied by the projection matrix.
        auto GetViewProjection() const -> const Matrix&;

        // param outPixel: Written only if the point is in front of the camera.
        // return: False if the point is behind the camera.
        auto ToScreen(const Vec3& world, Vec2& outPixel) const -> bool;

        // return: The ray from the eye through the pixel. The center ray if the viewport is empty.
        auto ScreenToRay(const Vec2& pixel) const -> Ray;

        // return: The ray from the eye along the forward direction.
        auto GetCenterRay() const -> Ray;

    private:
        CameraStore& m_CameraStore;
        OptionsStore& m_OptionsStore;
        GameCameraService& m_GameCameraService;

        std::optional<GameCamera> m_GameCamera{};

        // True while the free camera flies and moves the camera of the game (Free mode).
        bool m_IsFlying{ false };

        // The pose the director asked for and Update has not taken yet.
        bool m_IsScripted{ false };
        Vec3 m_ScriptedEye{};
        float m_ScriptedYaw{ 0.0f };
        float m_ScriptedPitch{ 0.0f };

        // Projection terms of the last Build: the focal values and the off-center terms of x and y.
        float m_FocalX{ 1.0f };
        float m_FocalY{ 1.0f };
        float m_CenterX{ 0.0f };
        float m_CenterY{ 0.0f };

        // Requested pose: m_Eye, m_Yaw and m_Pitch, with the axes they give. Render pose: m_RenderEye and m_Forward, m_Right and m_Up (see GetEye).
        Vec3 m_Eye{};
        Vec3 m_RenderEye{};
        Vec3 m_RequestedForward{ 1.0f, 0.0f, 0.0f };
        Vec3 m_RequestedUp{ 0.0f, 0.0f, 1.0f };
        float m_Yaw{ 0.0f };
        float m_Pitch{ 0.0f };
        Lens m_Lens{};

        Vec3 m_Forward{ 1.0f, 0.0f, 0.0f };
        Vec3 m_Right{ 0.0f, -1.0f, 0.0f };
        Vec3 m_Up{ 0.0f, 0.0f, 1.0f };

        Viewport m_Viewport{};
        Matrix m_ViewProjection{};

        bool m_IsActive{ false };
        bool m_IsLastUpdateSet{ false };
        bool m_IsFovSet{ false };

        steady_clock::time_point m_LastUpdate{};

        // Puts the eye on the local player and aims where it looks.
        auto SeatOnPlayer(const std::shared_ptr<const Tick>& tick) -> void;

        // Moves the field of view toward the one the mode and the zoom ask for.
        // note: The first call after the construction takes the target without a transition.
        auto UpdateLens(float deltaTime) -> void;

        // Turns the mouse movement into yaw and pitch, scaled by the field of view, and rebuilds the matrices.
        auto Look(float mouseX, float mouseY) -> void;

        // Sits the camera on the game camera and takes its axes. Its projection is used by Build.
        auto SeatOnGameCamera(const GameCamera& camera) -> void;

        // Turns with the mouse and moves with the keys.
        auto Fly(float deltaTime, float mouseX, float mouseY, bool acceptInput) -> void;

        // note: The pitch is clamped so the camera never flips.
        auto SetPose(const Vec3& eye, float yaw, float pitch) -> void;
        auto AimAlong(const Vec3& forward) -> void;

        // Recomputes the axes and the view projection matrix from the pose, the lens and the viewport.
        auto Build() -> void;
    };
}