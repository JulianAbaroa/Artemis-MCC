module Gui.Backend.System;
import :WndProc;

namespace Gui::Backend::System
{
    auto WndProcGuiService::HandleMessage(WindowMessage& message) -> bool
    {
        if (!m_Backend.IsInitialized()) return false;

        const bool isHandled = m_Backend.ForwardMessage(message);

        if (!m_SettingsStore.IsMenuVisible()) return false;

        return isHandled || m_Backend.WantsCapture(message.Message);
    }
}