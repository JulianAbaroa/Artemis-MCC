export module Gui.Backend.System:WndProc;

import :Backend;

import Service.Settings.State;
import Platform.Input.Type;
import std;

export namespace Gui::Backend::System
{
    // Gives the window messages to ImGui and tells if the game must not receive them.
    class WndProcGuiService
    {
    private:
        using SettingsStore = Service::Settings::State::SettingsStore;

        using WindowMessage = Platform::Input::Type::WindowMessage;

        using BackendGuiService = Gui::Backend::System::BackendGuiService;

    public:
        WndProcGuiService(BackendGuiService& backend, SettingsStore& settingsStore) :
            m_Backend(backend), m_SettingsStore(settingsStore) {}
        ~WndProcGuiService() = default;

        WndProcGuiService(const WndProcGuiService&) = delete;
        auto operator=(const WndProcGuiService&) -> WndProcGuiService& = delete;

        // Forwards the message to ImGui even when the menu is hidden, so it keeps track of the input.
        // return: True if the message was consumed. Always false while the menu is hidden.
        auto HandleMessage(WindowMessage& message) -> bool;

    private:
        BackendGuiService& m_Backend;
        SettingsStore& m_SettingsStore;
    };
}