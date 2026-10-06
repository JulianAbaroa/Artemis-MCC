export module UI.Launcher.System;

import Service.Settings.State;
import Platform.Render.Type;
import UI.Launcher.Type;
import UI.Launcher.State;
import UI.Icon.Type;
import std;

namespace
{
    using UI::Launcher::Type::k_TabCount;
}

export namespace UI::Launcher::System
{
    // Draws the launcher window and hosts the windows of the tabs.
    class LauncherUIService
    {
    private:
        using SettingsStore = Service::Settings::State::SettingsStore;

        using FrameContext = Platform::Render::Type::FrameContext;

        using Tab = UI::Launcher::Type::Tab;
        using TabContent = UI::Launcher::Type::TabContent;
        using LauncherUIStore = UI::Launcher::State::LauncherUIStore;
        using IconTexture = UI::Icon::Type::IconTexture;

    public:
        LauncherUIService(SettingsStore& settingsStore, LauncherUIStore& launcherStore) :
            m_SettingsStore(settingsStore), m_LauncherStore(launcherStore) {}
        ~LauncherUIService() = default;

        LauncherUIService(const LauncherUIService&) = delete;
        auto operator=(const LauncherUIService&) -> LauncherUIService& = delete;

        // Opens the launcher on the first call when the setting is enabled.
        auto OnInitialized() -> void;

        // Releases the icon textures.
        // note: Call it before the graphics device is released.
        auto ReleaseIcons() -> void;

        // Handles the requests of the hotkeys and updates the menu visibility in the settings.
        // return: true if the launcher or any tab is visible.
        auto Update() -> bool;

        // Draws the launcher window with the button of each tab.
        auto Draw(const FrameContext& frame) -> void;

        // Draws the full screen dock space that holds the tab windows.
        auto DrawDockSpace() -> void;

        // Draws the window of a tab when it is visible.
        // param content: Draws the content of the window.
        auto DrawTab(Tab tab, const char* title, const TabContent& content) -> void;

    private:
        SettingsStore& m_SettingsStore;
        LauncherUIStore& m_LauncherStore;

        std::array<IconTexture, k_TabCount> m_Icons{};
        bool m_IsIconsLoaded{ false };
        bool m_IsOpenOnStartHandled{ false };
        bool m_IsPreviouslyVisible{ false };

        // Loads the icon textures of the tabs.
        auto LoadIcons(const FrameContext& frame) -> void;

        // Draws the button that shows or hides a tab.
        auto DrawToggleButton(Tab tab) -> void;

        // Hides all the tabs or restores the ones hidden before.
        auto ToggleAllTabs() -> void;

        // Shows the visible tabs again and centers them.
        auto ResetTabs() -> void;
    };
}