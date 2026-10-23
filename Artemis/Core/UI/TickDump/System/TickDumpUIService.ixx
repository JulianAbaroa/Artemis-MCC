export module UI.TickDump.System;

import Common.Log.System;
import Service.Settings.State;
import Map.Reader.State;
import Export.Tick.Type;
import Export.Flat.System;
import UI.TickDump.Type;
import UI.TickDump.State;
import std;

export namespace UI::TickDump::System
{
    // Records the Tick of every tick to a .tdump file (D151): the BridgeTick that Export.Flat builds, written by the dump thread with
    // the layout of Bridge.xml embedded and the entities as a delta. Tools/tickdump.py reads the files.
    // note: Development tool (D72) outside the Domain pipeline. The AI thread only pushes a pointer to the immutable Tick, the dump
    // thread flattens and writes, and the render thread draws the panel.
    class TickDumpUIService
    {
    private:
        using milliseconds = std::chrono::milliseconds;

        using Log = Common::Log::System::Log;
        using SettingsStore = Service::Settings::State::SettingsStore;
        using FileStore = Map::Reader::State::FileStore;
        using Tick = Export::Tick::Type::Tick;
        using FlatService = Export::Flat::System::FlatService;
        using TickDumpEntry = UI::TickDump::Type::TickDumpEntry;
        using TickDumpUIStore = UI::TickDump::State::TickDumpUIStore;

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

        // Queues the Tick just assembled while a recording runs. AI thread.
        // param droppedBefore: ticks the AI thread dropped right before this one.
        // note: Waits while the queue is full. The game waits for the read up to its read timeout, after which the ticks it runs
        // count as dropped in the next record and in the footer (D151).
        auto Push(std::shared_ptr<const Tick> snapshot, std::uint64_t droppedBefore) -> void;

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
        // Writes what is still queued and the footer, and closes the file.
        auto Finish() -> void;
        // Ends a recording the file can no longer take (full disk, removed folder), without a footer.
        auto Fail() -> void;
        auto Write(const TickDumpEntry& entry) -> void;
    };
}