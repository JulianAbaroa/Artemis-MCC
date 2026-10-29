export module DevTools.Workbench.TickTrace.System;

import Common.Log.System;
import Services.Settings.State;
import Blam.Map.Reader.State;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Object.State;
import DevTools.Workbench.TickTrace.Type;
import DevTools.Workbench.TickTrace.State;
import std;

export namespace DevTools::Workbench::TickTrace::System
{
    // Records every tick to a binary trace file (A8 sec. 10.2): the engine clock and the bipeds, vehicles, projectiles and weapons of
    // the object table. Tools/analyze_ticktrace.py compares two traces tick by tick (V1).
    // note: Development tool (D72). The AI thread calls CaptureClock and Record on every tick, the UI thread writes the file.
    class TickTraceUIService
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Services::Settings::State::SettingsStore;
        using FileStore = Blam::Map::Reader::State::FileStore;
        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;
        using ObjectTableStore = Blam::Runtime::Object::State::ObjectTableStore;

        using TickClock = DevTools::Workbench::TickTrace::Type::TickClock;
        using TickTraceUIStore = DevTools::Workbench::TickTrace::State::TickTraceUIStore;

    public:
        TickTraceUIService(Log& log, SettingsStore& settingsStore, FileStore& fileStore,
            ObjectTableStore& objectStore, TickTraceUIStore& tickTraceUIStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_FileStore(fileStore),
            m_ObjectStore(objectStore), m_TickTraceUIStore(tickTraceUIStore) {}
        ~TickTraceUIService() = default;

        TickTraceUIService(const TickTraceUIService&) = delete;
        auto operator=(const TickTraceUIService&) -> TickTraceUIService& = delete;

        // Takes the engine clock of the current tick from the snapshot while recording.
        // note: AI thread, same copy as the tables.
        auto CaptureClock(const Snapshot& snapshot) -> void;

        // Appends the record of the current tick while recording.
        // param generation: Tick generation of Artemis.
        // note: AI thread, after the object table of the tick is published.
        auto Record(std::uint64_t generation) -> void;

        // Starts or stops the recording if another thread asked for it, then writes the pending records to the file.
        // note: UI thread, every frame, also with the UI hidden.
        auto Update() -> void;

        // Stops the recording and closes the file. Does nothing if not recording.
        auto Stop() -> void;

        // Draws the tick trace section of the simulation tab.
        auto Draw() -> void;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        FileStore& m_FileStore;
        ObjectTableStore& m_ObjectStore;
        TickTraceUIStore& m_TickTraceUIStore;

        // Opens a new trace file in the storage folder, writes its header and starts recording.
        auto Start() -> void;

        // return: The clock of the engine, invalid if the snapshot has no thread context or time globals.
        auto ReadClock(const Snapshot& snapshot) const -> TickClock;
    };
}