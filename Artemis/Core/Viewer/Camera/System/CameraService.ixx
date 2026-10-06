export module Viewer.Camera.System;

import Export.Tick.Type;
import Viewer.Camera.Type;
import Viewer.Camera.State;
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
        using Vec2 = Viewer::Camera::Type::Vec2;
        using Vec3 = Viewer::Camera::Type::Vec3;
        using Matrix = Viewer::Camera::Type::Matrix;
        using Lens = Viewer::Camera::Type::Lens;
        using Viewport = Viewer::Camera::Type::Viewport;
        using Ray = Viewer::Camera::Type::Ray;
        using CameraStore = Viewer::Camera::State::CameraStore;

    public:
        explicit CameraService(CameraStore& cameraStore) :
            m_CameraStore(cameraStore) {}
        ~CameraService() = default;

        CameraService(const CameraService&) = delete;
        auto operator=(const CameraService&) -> CameraService& = delete;

        // Advances the camera and rebuilds the matrices.
        // param acceptInput: If false, the pressed keys are released and the camera does not fly.
        // param followHandle: Object to follow. Used only if the follow flag is set and the object is in the tick.
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
        steady_clock::time_point m_LastUpdate{};

        // Puts the eye on the local player and aims where it looks.
        auto SeatOnPlayer(const std::shared_ptr<const Tick>& tick) -> void;

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