export module DevTools.Viewer.Camera.State;

import DevTools.Viewer.Camera.Type;
import std;

export namespace DevTools::Viewer::Camera::State
{
    // Holds the camera input: the active and zoom flags, the mode, the follow mode, the orbit distance, the pressed keys and the accumulated mouse movement.
    // The values are atomic because the window messages write them and the render frame reads them.
    class CameraStore
    {
    private:
        using Key = DevTools::Viewer::Camera::Type::Key;
        using Mode = DevTools::Viewer::Camera::Type::Mode;
        using Follow = DevTools::Viewer::Camera::Type::Follow;

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

        // Reads, sets or toggles the zoom of the Free mode.
        auto IsZoomActive() const -> bool;
        auto SetZoomActive(bool value) -> void;
        auto ToggleZoom() -> void;

        auto GetFollow() const -> Follow;
        auto SetFollow(Follow follow) -> void;

        // Advances the follow mode: Off -> FirstPerson -> Orbit -> Off.
        auto CycleFollow() -> void;

        // Reads or sets the distance from the camera to the orbited object, in world units.
        // note: The value is clamped to the allowed range.
        auto GetOrbitDistance() const -> float;
        auto SetOrbitDistance(float value) -> void;

        auto IsKeyDown(Key key) const -> bool;
        auto SetKeyDown(Key key, bool down) -> void;

        // Releases every key.
        auto ResetKeys() -> void;

        // Adds a mouse movement to the one not consumed yet.
        auto AccumulateMouseDelta(float dx, float dy) -> void;

        // Returns the accumulated mouse movement and resets it to zero.
        auto ConsumeMouseDelta(float& outX, float& outY) -> void;

    private:
        static constexpr float k_MinOrbitDistance{ 0.5f };
        static constexpr float k_MaxOrbitDistance{ 100.0f };

        static constexpr std::size_t k_KeyCount{ static_cast<std::size_t>(Key::Count) };

        std::atomic<bool> m_IsActive{ false };
        std::atomic<Mode> m_Mode{ Mode::Free };
        std::atomic<bool> m_IsZoomActive{ false };
        std::atomic<Follow> m_Follow{ Follow::Off };
        std::atomic<float> m_OrbitDistance{ 5.0f };

        std::array<std::atomic<bool>, k_KeyCount> m_Keys{};

        std::atomic<float> m_MouseDeltaX{ 0.0f };
        std::atomic<float> m_MouseDeltaY{ 0.0f };
    };
}