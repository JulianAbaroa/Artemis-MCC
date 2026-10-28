export module Viewer.Overlay.State;

import Viewer.Overlay.Type;
import std;

export namespace Viewer::Overlay::State
{
    // Holds the active overlay mode, the page of its panel and the visibility of the overlay.
    // The values are atomic so they can be read and written from different threads without locks.
    class OverlayStore
    {
    private:
        using Mode = Viewer::Overlay::Type::Mode;

    public:
        OverlayStore() = default;
        ~OverlayStore() = default;

        auto GetMode() const -> Mode;

        // note: The modes wrap around. Changing the mode resets the page.
        auto NextMode() -> void;
        auto PreviousMode() -> void;
        auto ResetMode() -> void;

        auto GetPage() const -> int;
        auto NextPage() -> void;
        auto PreviousPage() -> void;
        auto ResetPage() -> void;

        // Keeps the page between 0 and maxPage.
        auto ClampPage(int maxPage) -> void;

        auto IsVisible() const -> bool;
        auto SetVisible(bool value) -> void;
        auto ToggleVisible() -> void;

    private:
        std::atomic<std::uint8_t> m_Mode{ static_cast<std::uint8_t>(Mode::Default) };
        std::atomic<int> m_Page{ 0 };
        std::atomic<bool> m_IsVisible{ false };
    };
}