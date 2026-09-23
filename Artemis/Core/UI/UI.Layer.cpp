module UI.Layer;

import UI.Launcher.Type;
import std;

namespace
{
	using Tab = UI::Launcher::Type::Tab;
}

namespace UI
{
	auto Layer::Initialize(const FrameContext& frame) -> void
	{
		m_LauncherUIService.ReleaseIcons();

		m_BackendUIService.Initialize(frame);
		m_LauncherUIService.OnInitialized();
	}

	auto Layer::Shutdown() -> void
	{
		m_LauncherUIService.ReleaseIcons();
		m_BackendUIService.Shutdown();

		m_ObjectTableUIStore.Cleanup();
		m_PlayerTableUIStore.Cleanup();
	}

	auto Layer::DrawFrame(const FrameContext& frame) -> void
	{
		if (!m_BackendUIService.IsReady()) return;

		const bool isMenuVisible = m_LauncherUIService.Update();

		m_BackendUIService.NewFrame(isMenuVisible);

		if (isMenuVisible) m_LauncherUIService.DrawDockSpace();

		m_OverlayUIService.Draw();

		m_LauncherUIService.DrawTab(Tab::ObjectTable, "Object Table", [this] { m_ObjectTableUIService.Draw(); });
		m_LauncherUIService.DrawTab(Tab::PlayerTable, "Player Table", [this] { m_PlayerTableUIService.Draw(); });
		m_LauncherUIService.DrawTab(Tab::Settings, "Settings", [this] { m_SettingsUIService.Draw(); });
		m_LauncherUIService.DrawTab(Tab::MemoryScanner, "Memory Scanner", [this] { m_MemoryScannerUIService.Draw(); });
		m_LauncherUIService.DrawTab(Tab::Logs, "Logs", [this] { m_LogsUIService.Draw(); });

		m_LauncherUIService.Draw(frame);

		m_BackendUIService.Render();
	}
}