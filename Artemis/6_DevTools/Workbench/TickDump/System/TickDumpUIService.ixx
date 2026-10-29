export module DevTools.Workbench.TickDump.System;

import Common.Log.System;
import Services.Settings.State;
import Blam.Map.Reader.State;
import Blam.Layout.Snapshot.Type;
import Bridge.Codec.Tick.Type;
import Bridge.Codec.Flat.System;
import DevTools.Workbench.TickDump.Type;
import DevTools.Workbench.TickDump.State;
import std;

export namespace DevTools::Workbench::TickDump::System
{
    // Records every tick to a .tdump file (D151) and a .sdump file with the same name (D202): the BridgeTick that Bridge.Codec.Flat builds,
    // written by the dump thread with the layout of Bridge.xml embedded and the entities as a delta, and the Snapshot the tick was
    // built from, as a delta of bytes. Tools/tickdump.py reads the .tdump and the tests replay the .sdump.
    // note: Development tool (D72) outside the Domain pipeline. The AI thread only pushes a pointer to the immutable Tick, the dump
    // thread flattens and writes, and the render thread draws the panel.
    class TickDumpUIService
    {
    private:
        using milliseconds = std::chrono::milliseconds;

        using Log = Common::Log::System::Log;
        using SettingsStore = Services::Settings::State::SettingsStore;
        using FileStore = Blam::Map::Reader::State::FileStore;
        using Snapshot = Blam::Layout::Snapshot::Type::Snapshot;
        using Tick = Bridge::Codec::Tick::Type::Tick;
        using FlatService = Bridge::Codec::Flat::System::FlatService;
        using TickDumpEntry = DevTools::Workbench::TickDump::Type::TickDumpEntry;
        using TickDumpUIStore = DevTools::Workbench::TickDump::State::TickDumpUIStore;

    public:
        TickDumpUIService(Log& log, SettingsStore& settingsStore, FileStore& fileStore, FlatService& flatService,
            TickDumpUIStore& tickDumpUIStore) :
            m_Log(log), m_SettingsStore(settingsStore), m_FileStore(fileStore), m_FlatService(flatService),
            m_TickDumpUIStore(tickDumpUIStore) {}
        ~TickDumpUIService() = default;

        TickDumpUIService(const TickDumpUIService&) = delete;
        auto operator=(const TickDumpUIService&) -> TickDumpUIService& = delete;

        // Whether a recording runs, so the AI thread only takes the Tick when it is needed.
        auto IsRecording() const -> bool;

        // Queues the Tick just assembled and the Snapshot it came from while a recording runs. AI thread.
        // param droppedBefore: ticks the AI thread dropped right before this one.
        // param isFirstAfterLoad: The tick is the first one that ran after the map loaded.
        // param isViewerActive: The viewer camera was active in the tick.
        // note: Waits while the queue is full. The game waits for the read up to its read timeout, after which the ticks it runs
        // count as dropped in the next record and in the footer (D151).
        auto Push(std::shared_ptr<const Tick> tick, std::shared_ptr<const Snapshot> snapshot, std::uint64_t generation,
            std::uint64_t droppedBefore, bool isFirstAfterLoad, bool isViewerActive) -> void;

        // Takes the requests of the UI and writes the queued Ticks, waiting up to the timeout for the first one. Dump thread.
        auto Update(milliseconds timeout) -> void;

        // Ends a running recording with its footer. Dump thread, when the lifecycle stops.
        auto Shutdown() -> void;

        // Panel of the Simulation tab. Render thread.
        auto Draw() -> void;

    private:
        Log& m_Log;
        SettingsStore& m_SettingsStore;
        FileStore& m_FileStore;
        FlatService& m_FlatService;
        TickDumpUIStore& m_TickDumpUIStore;

        auto Start(std::string_view source) -> void;
        // Writes what is still queued and the footers, and closes the files.
        auto Finish() -> void;
        // Ends a recording a file can no longer take (full disk, removed folder), without footers.
        auto Fail() -> void;
        auto Write(const TickDumpEntry& entry) -> void;
    };
}