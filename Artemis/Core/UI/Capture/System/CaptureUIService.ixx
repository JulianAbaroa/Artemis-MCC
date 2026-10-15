export module UI.Capture.System;

import Service.Logs.System;
import Service.Settings.State;
import Platform.Memory.System;
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
    // Each object also gets its Havok component and rigid bodies, and each capture the layer table of the Havok collision filter (A10).
    // note: A development tool (D72). The datum comes from the copy DatumService already makes each tick. The Havok lines are read live
    // from the render thread, through the safe reader, at fixed RVAs of haloreach.dll (Ignore/Tareas/Transversal/A10_Colisiones.txt).
    class CaptureUIService
    {
    private:
        using LogsService = Service::Logs::System::LogsService;
        using SettingsStore = Service::Settings::State::SettingsStore;

        using MemoryReaderService = Platform::Memory::System::MemoryReaderService;

        using Snapshot = Datum::Object::Type::Raw::Snapshot;
        using DatumStore = Datum::Object::State::DatumStore;

        using Tick = Export::Tick::Type::Tick;
        using TickStore = Export::Tick::State::TickStore;

        using Target = UI::Capture::Type::Target;
        using HavokFilter = UI::Capture::Type::HavokFilter;
        using HavokComponent = UI::Capture::Type::HavokComponent;
        using CaptureUIStore = UI::Capture::State::CaptureUIStore;

    public:
        CaptureUIService(LogsService& logsService, SettingsStore& settingsStore, MemoryReaderService& memoryReaderService,
            DatumStore& datumStore, TickStore& tickStore, CaptureUIStore& captureUIStore) :
            m_LogsService(logsService), m_SettingsStore(settingsStore), m_MemoryReaderService(memoryReaderService),
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
        MemoryReaderService& m_MemoryReaderService;
        DatumStore& m_DatumStore;
        TickStore& m_TickStore;
        CaptureUIStore& m_CaptureUIStore;

        // Appends one capture to the session file, keeps its diff against the previous capture and logs a summary of it.
        // return: The status text shown in the window.
        auto Capture() -> std::string;

        // Lists the objects of a capture, each handle once: the local biped, every other biped if asked, the pinned objects,
        // and the weapons, armor ability and vehicle of each of those bipeds.
        auto CollectTargets(const Tick& tick, const Snapshot& snapshot) const -> std::vector<Target>;

        // Reads the Havok world and the layer table of its collision filter.
        // return: Nothing if haloreach.dll, the world or the filter is not found.
        auto ReadHavokFilter() -> std::optional<HavokFilter>;

        // Reads one Havok component and its rigid bodies.
        // param handle: Value of obje 0xCC.
        // return: Nothing if the handle is empty or the component data array is not found.
        auto ReadHavokComponent(std::uint32_t handle) -> std::optional<HavokComponent>;

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