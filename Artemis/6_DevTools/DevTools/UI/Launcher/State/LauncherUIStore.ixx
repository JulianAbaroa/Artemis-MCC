export module UI.Launcher.State;

import UI.Launcher.Type;
import std;

namespace
{
    using UI::Launcher::Type::k_TabCount;
}

export namespace UI::Launcher::State
{
    // Visibility state of the launcher and its tabs.
    // note: Atomic because the hotkeys change it from the window thread and the UI reads it from the render thread.
    class LauncherUIStore
    {
    private:
        using Tab = UI::Launcher::Type::Tab;

    public:
        LauncherUIStore() = default;
        ~LauncherUIStore() = default;

        LauncherUIStore(const LauncherUIStore&) = delete;
        auto operator=(const LauncherUIStore&) -> LauncherUIStore& = delete;

        // Checks whether the launcher window is visible.
        auto IsVisible() const -> bool;

        // Shows or hides the launcher window.
        auto SetVisible(bool value) -> void;

        // Flips the visibility of the launcher window.
        auto ToggleVisible() -> void;

        // Checks whether the window of the tab is visible.
        auto IsTabVisible(Tab tab) const -> bool;

        // Shows or hides the window of the tab.
        auto SetTabVisible(Tab tab, bool value) -> void;

        // Flips the visibility of the window of the tab.
        auto ToggleTab(Tab tab) -> void;

        // Checks whether at least one tab window is visible.
        auto IsAnyTabVisible() const -> bool;

        // Asks the tab window to reset its position and size.
        auto RequestTabReset(Tab tab) -> void;

        // Reads and clears the reset request of the tab.
        // return: true if a reset was requested.
        auto ConsumeTabReset(Tab tab) -> bool;

        // Asks the launcher to hide or restore all the tab windows.
        auto RequestToggleAllTabs() -> void;

        // Reads and clears the toggle all request.
        // return: true if a toggle was requested.
        auto ConsumeToggleAllTabs() -> bool;

        // Stores which tab windows are visible now.
        auto SaveTabSnapshot() -> void;

        // Checks whether a snapshot of the tab windows was saved.
        auto IsTabSnapshotSaved() const -> bool;

        // Checks whether the tab window was visible when the snapshot was saved.
        auto WasTabVisible(Tab tab) const -> bool;

    private:
        std::atomic<bool> m_IsVisible{ false };

        std::array<std::atomic<bool>, k_TabCount> m_TabVisible{};
        std::array<std::atomic<bool>, k_TabCount> m_TabResetRequested{};

        std::atomic<bool> m_ToggleAllRequested{ false };

        std::array<bool, k_TabCount> m_TabSnapshot{};
        bool m_IsTabSnapshotSaved{ false };
    };
}