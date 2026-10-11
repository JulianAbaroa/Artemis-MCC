export module Viewer.Camera.State;

import Viewer.Camera.Type;
import std;

export namespace Viewer::Camera::State
{
    // Holds the camera input: the active and follow flags, the mode, the pressed keys and the accumulated mouse movement.
    // The values are atomic because the window messages write them and the render frame reads them.
    class CameraStore
    {
    private:
        using Key = Viewer::Camera::Type::Key;
        using Mode = Viewer::Camera::Type::Mode;

    public:
        CameraStore() = default;
        ~CameraStore() = default;

        CameraStore(const CameraStore&) = delete;
        auto operator=(const CameraStore&) -> CameraStore& = delete;

        auto IsActive() const -> bool;
        auto SetActive(bool value) -> void;

        auto GetMode() const -> Mode;

        // Changes the mode. A real change releases every key and drops the pending mouse movement.
        auto SetMode(Mode mode) -> void;

        // Switches between the Free and the Game mode.
        auto ToggleMode() -> void;

        auto IsFollowEnabled() const -> bool;
        auto SetFollowEnabled(bool value) -> void;

        auto IsKeyDown(Key key) const -> bool;
        auto SetKeyDown(Key key, bool down) -> void;

        // Releases every key.
        auto ResetKeys() -> void;

        // Adds a mouse movement to the one not consumed yet.
        auto AccumulateMouseDelta(float dx, float dy) -> void;

        // Returns the accumulated mouse movement and resets it to zero.
        auto ConsumeMouseDelta(float& outX, float& outY) -> void;

    private:
        static constexpr std::size_t k_KeyCount{ static_cast<std::size_t>(Key::Count) };

        std::atomic<bool> m_IsActive{ false };
        std::atomic<Mode> m_Mode{ Mode::Free };
        std::atomic<bool> m_IsFollowEnabled{ false };

        std::array<std::atomic<bool>, k_KeyCount> m_Keys{};

        std::atomic<float> m_MouseDeltaX{ 0.0f };
        std::atomic<float> m_MouseDeltaY{ 0.0f };
    };
}