export module UI.Capture.System;

import Service.Logs.System;
import Service.Settings.State;
import Datum.Object.Type;
import Datum.Object.State;
import Export.Tick.Type;
import Export.Tick.State;
import UI.Capture.Type;
import UI.Capture.State;
import std;

export namespace UI::Capture::System
{
    // Copies the datum of the local biped, its linked objects and the pinned objects to a session file, cut by component (D81).
    // note: A development tool (D72). It reads the copy DatumService already makes each tick, so a capture adds no memory reads.
    class CaptureUIService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using Snapshot = Datum::Object::Type::Raw::Snapshot;
        using DatumStore = Datum::Object::State::DatumStore;

        using Tick = Export::Tick::Type::Tick;
        using TickStore = Export::Tick::State::TickStore;

        using Target = UI::Capture::Type::Target;
        using CaptureUIStore = UI::Capture::State::CaptureUIStore;

    public:
        CaptureUIService(LogsService& logsService, SettingsStore& settingsStore,
            DatumStore& datumStore, TickStore& tickStore, CaptureUIStore& captureUIStore) :
            m_LogsService(logsService), m_SettingsStore(settingsStore),
            m_DatumStore(datumStore), m_TickStore(tickStore), m_CaptureUIStore(captureUIStore) {}
        ~CaptureUIService() = default;

        CaptureUIService(const CaptureUIService&) = delete;
        auto operator=(const CaptureUIService&) -> CaptureUIService& = delete;

        // Runs the capture asked by the hotkey or the button.
        // note: Called from the render thread on every frame, also while the window is hidden.
        auto Update() -> void;

        // Draws the capture window content.
        auto Draw() -> void;

    private:
        LogsService& m_LogsService;
        SettingsStore& m_SettingsStore;
        DatumStore& m_DatumStore;
        TickStore& m_TickStore;
        CaptureUIStore& m_CaptureUIStore;

        // Appends one capture to the session file, keeps its diff against the previous capture and logs a summary of it.
        // return: The status text shown in the window.
        auto Capture() -> std::string;

        // Lists the objects of a capture, each handle once: the local biped, every other biped if asked, the pinned objects,
        // and the weapons, armor ability and vehicle of each of those bipeds.
        auto CollectTargets(const Tick& tick, const Snapshot& snapshot) const -> std::vector<Target>;

        // Opens the session file to append. The first capture of a session creates it in the Captures folder of the storage folder.
        // return: The open file, or nothing if the storage folder is not set or the file cannot be opened.
        auto OpenSessionFile() -> std::optional<std::ofstream>;

        // Draws the label, the options and the buttons.
        auto DrawControls() -> void;

        // Draws the pinned objects with a button to unpin each one.
        auto DrawPinned() -> void;

        // Draws the byte runs that changed in the last capture, filtered by component.
        auto DrawChanges() -> void;
    };
}