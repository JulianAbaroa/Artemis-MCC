export module Viewer.Camera.System;

import Common.Math.Type;
import Export.Tick.Type;
import Viewer.Camera.Type;
import Viewer.Camera.State;
import Viewer.Options.State;
import std;

export namespace Viewer::Camera::System
{
    // Free-fly camera of the viewer. Moves with the keys and the mouse, or follows an object.
    // It builds the view and projection matrices on every update and projects between the world and the screen.
    class CameraService
    {
    private:
        using steady_clock = std::chrono::steady_clock;
        using Tick = Export::Tick::Type::Tick;
        using Vec2 = Common::Math::Type::Vec2;
        using Vec3 = Common::Math::Type::Vec3;
        using Matrix = Common::Math::Type::Matrix;
        using Lens = Viewer::Camera::Type::Lens;
        using Viewport = Viewer::Camera::Type::Viewport;
        using Ray = Viewer::Camera::Type::Ray;
        using CameraStore = Viewer::Camera::State::CameraStore;
        using OptionsStore = Viewer::Options::State::OptionsStore;

    public:
        CameraService(CameraStore& cameraStore, OptionsStore& optionsStore) :
            m_CameraStore(cameraStore), m_OptionsStore(optionsStore) {}
        ~CameraService() = default;

        CameraService(const CameraService&) = delete;
        auto operator=(const CameraService&) -> CameraService& = delete;

        // Advances the camera and rebuilds the matrices.
        // note: In Game mode the camera sits on the local player view and keeps its last pose while the player is dead.
        // param acceptInput: If false, the pressed keys are released and the camera does not fly.
        // param followHandle: Object to follow or orbit. Used only if the follow mode of the store asks for it and the object is in the tick.
        // note: The first update after Deactivate places the camera on the local player.
        auto Update(const std::shared_ptr<const Tick>& tick,
            const Viewport& viewport, bool acceptInput,
            std::optional<std::uint32_t> followHandle) -> void;

        // Makes the next Update place the camera on the local player again.
        auto Deactivate() -> void;

        auto GetEye() const -> Vec3;
        auto GetForward() const -> Vec3;
        auto GetRight() const -> Vec3;
        auto GetUp() const -> Vec3;
        auto GetYaw() const -> float;
        auto GetPitch() const -> float;

        auto GetViewport() const -> const Viewport&;

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

        Vec3 m_Eye{};
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

        // True if the last update orbited an object.
        bool m_IsOrbiting{ false };
        steady_clock::time_point m_LastUpdate{};

        // Puts the eye on the local player and aims where it looks.
        auto SeatOnPlayer(const std::shared_ptr<const Tick>& tick) -> void;

        // Moves the field of view toward the one the mode and the zoom ask for.
        // note: The first call after the construction takes the target without a transition.
        auto UpdateLens(float deltaTime) -> void;

        // Turns the mouse movement into yaw and pitch, scaled by the field of view, and rebuilds the matrices.
        auto Look(float mouseX, float mouseY) -> void;

        // return: False if the object is not a collidable nor the living biped of a player.
        auto FindPivot(const std::shared_ptr<const Tick>& tick,
            std::uint32_t selected, Vec3& outPivot) const -> bool;

        // Turns around the object at the orbit distance. The mouse rotates, the wheel (ControlService) sets the distance.
        // param wasOrbiting: False on the first frame, when the distance and the aim come from the current pose.
        // return: False if the object is not in the tick, and the camera does not move.
        auto OrbitAround(const std::shared_ptr<const Tick>& tick, std::uint32_t selected,
            bool wasOrbiting, float mouseX, float mouseY, bool acceptInput) -> bool;

        // Places the camera on the local player view. Keeps the last pose if there is no living local player.
        auto FollowSelf(const std::shared_ptr<const Tick>& tick) -> void;

        // return: False if the object is neither a player biped nor a collidable of the tick.
        auto FindFollowTarget(const std::shared_ptr<const Tick>& tick,
            std::uint32_t handle, Vec3& outPosition, Vec3& outForward) const -> bool;

        // Turns with the mouse and moves with the keys.
        auto Fly(float deltaTime, float mouseX, float mouseY, bool acceptInput) -> void;

        // note: The pitch is clamped so the camera never flips.
        auto SetPose(const Vec3& eye, float yaw, float pitch) -> void;
        auto AimAlong(const Vec3& forward) -> void;

        // Recomputes the axes and the view projection matrix from the pose, the lens and the viewport.
        auto Build() -> void;
    };
}