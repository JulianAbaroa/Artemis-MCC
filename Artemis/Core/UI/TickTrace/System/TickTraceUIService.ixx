export module UI.TickTrace.System;

import Common.Log.System;
import Service.Settings.State;
import Map.Reader.State;
import Datum.Object.State;
import Tables.Object.State;
import UI.TickTrace.Type;
import UI.TickTrace.State;
import std;

export namespace UI::TickTrace::System
{
    // Records every tick to a binary trace file (A8 sec. 10.2): the engine clock and the bipeds, vehicles, projectiles and weapons of
    // the object table. Tools/analyze_ticktrace.py compares two traces tick by tick (V1).
    // note: Development tool (D72). The AI thread calls CaptureClock and Record on every tick, the UI thread writes the file.
    class TickTraceUIService
    {
    private:
        using Log = Common::Log::System::Log;
        using SettingsStore = Service::Settings::State::SettingsStore;
        using FileStore = Map::Reader::State::FileStore;
        using DatumStore = Datum::Object::State::DatumStore;
        using ObjectTableStore = Tables::Object::State::ObjectTableStore;

        using TickClock = UI::TickTrace::Type::TickClock;
        using TickTraceUIStore = UI::TickTrace::State::TickTraceUIStore;

    public:
        TickTraceUIService(Log& log, SettingsStore& settingsStore, FileStore& fileStore, DatumStore& datumStore,
            ObjectTableStore& objectStore, TickTraceUIStore& tickTraceUIStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_FileStore(fileStore), m_DatumStore(datumStore),
            m_ObjectStore(objectStore), m_TickTraceUIStore(tickTraceUIStore) {}
        ~TickTraceUIService() = default;

        TickTraceUIService(const TickTraceUIService&) = delete;
        auto operator=(const TickTraceUIService&) -> TickTraceUIService& = delete;

        // Reads the engine clock of the current tick while recording.
        // note: AI thread, inside the read window (before LifecycleStore::EndRead), so the clock matches the copied tables.
        auto CaptureClock() -> void;

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
        DatumStore& m_DatumStore;
        ObjectTableStore& m_ObjectStore;
        TickTraceUIStore& m_TickTraceUIStore;

        // Opens a new trace file in the storage folder, writes its header and starts recording.
        auto Start() -> void;

        // return: The clock of the engine, invalid if the thread context or the time globals are not available.
        auto ReadClock() const -> TickClock;
    };
}