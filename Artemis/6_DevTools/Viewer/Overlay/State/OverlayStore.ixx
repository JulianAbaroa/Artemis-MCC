export module DevTools.Viewer.Overlay.State;

import std;

export namespace DevTools::Viewer::Overlay::State
{
    // Holds the visibility of the telemetry overlay.
    // The value is atomic so it can be read and written from different threads without locks.
    class OverlayStore
    {
    public:
        OverlayStore() = default;
        ~OverlayStore() = default;

        auto IsVisible() const -> bool;
        auto SetVisible(bool value) -> void;
        auto ToggleVisible() -> void;

    private:
        std::atomic<bool> m_IsVisible{ false };
    };
}