module UI.Backend.System;
import :WndProc;

namespace UI::Backend::System
{
	auto WndProcUIService::HandleMessage(WindowMessage& message) -> bool
	{
		if (!m_Backend.IsInitialized()) return false;

		const bool isHandled = m_Backend.ForwardMessage(message);

		if (!m_SettingsStore.IsMenuVisible()) return false;

		return isHandled || m_Backend.WantsCapture(message.Message);
	}
}