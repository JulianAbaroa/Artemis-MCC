module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

module UI.Hotkey.System;

import std;

namespace
{
    // Checks the auto repeat bit of a key message.
    auto IsRepeat(std::intptr_t lParam) -> bool
    {
        return (lParam & 0x40000000) != 0;
    }

    // Checks whether the Shift key is pressed.
    auto IsShiftDown() -> bool
    {
        return (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    }
}

namespace UI::Hotkey::System
{
    auto HotkeyUIService::HandleMessage(WindowMessage& message) -> bool
    {
        const bool isKeyDown{ message.Message == WM_KEYDOWN ||
            message.Message == WM_SYSKEYDOWN };

        if (!isKeyDown) return false;

        const auto virtualKey{ static_cast<std::uint32_t>(message.WParam) };
        const auto* binding{ m_HotkeyStore.Find(virtualKey, IsShiftDown()) };
        if (binding == nullptr) return false;

        if (!IsRepeat(message.LParam)) this->Execute(binding->Action);

        message.Result = 0;
        return true;
    }

    auto HotkeyUIService::Execute(Action action) -> void
    {
        switch (action)
        {
        case Action::ToggleLauncher:
            m_LauncherStore.ToggleVisible();
            break;

        case Action::ToggleOverlay:
            m_OverlayStore.ToggleVisible();
            break;

        case Action::ToggleMenu:
            m_LauncherStore.RequestToggleAllTabs();
            break;

        case Action::ResetMenu:
            m_SettingsStore.SetMenuResetPending(true);
            break;

        case Action::LockMenu:
            m_SettingsStore.SetMenuLocked(!m_SettingsStore.IsMenuLocked());
            break;

        default:
            break;
        }
    }
}