module UI.Layer;

import UI.Launcher.Type;
import std;

namespace
{
    using Tab = UI::Launcher::Type::Tab;
}

namespace UI
{
    auto Layer::Initialize(const FrameContext&) -> void
    {
        m_LauncherUIService.ReleaseIcons();
        m_LauncherUIService.OnInitialized();
    }

    auto Layer::Shutdown() -> void
    {
        m_LauncherUIService.ReleaseIcons();
        m_ObjectTableUIStore.Cleanup();
        m_PlayerTableUIStore.Cleanup();
    }

    auto Layer::DrawFrame(const FrameContext& frame) -> void
    {
        // The capture hotkey works with the UI closed, so it runs before the backend check.
        m_CaptureUIService.Update();

        if (!m_BackendGuiService.IsReady()) return;

        const bool isMenuVisible{ m_LauncherUIService.Update() };

        if (isMenuVisible) m_LauncherUIService.DrawDockSpace();

        m_LauncherUIService.DrawTab(Tab::ObjectTable, "Object Table", [this] {
            m_ObjectTableUIService.Draw();
        });
        m_LauncherUIService.DrawTab(Tab::PlayerTable, "Player Table", [this] {
            m_PlayerTableUIService.Draw();
        });
        m_LauncherUIService.DrawTab(Tab::DefinitionsInspector, "Definitions Inspector", [this] {
            m_DefinitionsInspectorUIService.Draw();
        });
        m_LauncherUIService.DrawTab(Tab::Settings, "Settings", [this] {
            m_SettingsUIService.Draw();
        });
        m_LauncherUIService.DrawTab(Tab::Capture, "Datum Capture", [this] {
            m_CaptureUIService.Draw();
        });
        m_LauncherUIService.DrawTab(Tab::Simulation, "Simulation", [this] {
            m_SimulationUIService.Draw();
        });
        m_LauncherUIService.DrawTab(Tab::Logs, "Logs", [this] {
            m_LogsUIService.Draw();
        });

        m_LauncherUIService.Draw(frame);
    }
}