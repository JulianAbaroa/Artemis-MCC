module;

#include "External/imgui/imgui.h"

module DevTools.Workbench.TickDump.System;

import Common.Hash.System;
import Common.Text.System;
import Common.Time.System;
import Bridge.Transport.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Layout.Snapshot.System;
import Bridge.Codec.Dump.Type;
import Bridge.Codec.Dump.System;
import DevTools.Workbench.TickDump.Type;
import std;

namespace
{
    using std::filesystem::path;
    using Common::Hash::System::FileCrc32;
    using Common::Text::System::CopyText;
    using Common::Time::System::LocalNow;

    using Contract::Bridge::Type::BridgeTick;

    using Bridge::Codec::Dump::Type::TickDumpCounters;
    using Blam::Layout::Snapshot::Type::SnapshotDumpFooter;
    using Blam::Layout::Snapshot::Type::SnapshotLoadInfo;

    using Common::Text::System::k_BytesPerMegabyte;
    using Contract::Bridge::Type::k_BridgeLayoutHash;
    using Blam::Layout::Snapshot::Type::k_SnapshotDumpEndMarker;
    using Blam::Layout::Snapshot::Type::k_SnapshotFirstAfterLoad;
    using Blam::Layout::Snapshot::Type::k_SnapshotViewerActive;

    // Subfolder of the storage folder that holds the dumps.
    constexpr std::string_view k_DumpFolder{ "TickDumps" };

    // Records written by one Update at most, so the requests of the UI are seen even when the queue never empties.
    constexpr std::size_t k_MaxRecordsPerUpdate{ 600 };

    // return: The bytes of both files in MB.
    auto ToMegabytes(std::uint64_t bytes) -> double
    {
        return static_cast<double>(bytes) / k_BytesPerMegabyte;
    }

    // return: The name of the map file without its folder, the size of the file (0 if it cannot be read) and its CRC-32 if computed.
    auto DescribeMap(const std::string& mapFilePath, bool withCrc) -> SnapshotDumpFooter
    {
        SnapshotDumpFooter footer{};
        if (mapFilePath.empty()) return footer;

        const path mapPath{ mapFilePath };
        CopyText(mapPath.filename().string(), footer.MapFileName);

        std::error_code error{};
        const std::uintmax_t size{ std::filesystem::file_size(mapPath, error) };
        if (!error) footer.MapFileSize = size;

        if (withCrc)
        {
            if (const auto crc{ FileCrc32(mapPath) })
            {
                footer.MapCrc32 = *crc;
                footer.HasCrc = 1;
            }
        }
        return footer;
    }
}

namespace DevTools::Workbench::TickDump::System
{
    auto TickDumpUIService::IsRecording() const -> bool
    {
        return m_TickDumpUIStore.IsRecording();
    }

    auto TickDumpUIService::Push(std::shared_ptr<const Tick> tick, std::shared_ptr<const Snapshot> snapshot,
        std::uint64_t generation, std::uint64_t droppedBefore, bool isFirstAfterLoad, bool isViewerActive) -> void
    {
        if (!m_TickDumpUIStore.IsRecording() || tick == nullptr || snapshot == nullptr) return;

        m_TickDumpUIStore.Push(TickDumpEntry{ .Tick = std::move(tick), .Snapshot = std::move(snapshot), .Generation = generation,
            .DroppedBefore = static_cast<std::uint32_t>((std::min)(droppedBefore, std::uint64_t{ (std::numeric_limits<std::uint32_t>::max)() })),
            .IsFirstAfterLoad = isFirstAfterLoad, .IsViewerActive = isViewerActive });
    }

    auto TickDumpUIService::Update(milliseconds timeout) -> void
    {
        if (auto source{ m_TickDumpUIStore.TakeStartRequest() }) this->Start(*source);
        if (m_TickDumpUIStore.TakeStopRequest())
        {
            this->Finish();
            return;
        }

        auto entry{ m_TickDumpUIStore.WaitPop(timeout) };
        for (std::size_t written{ 0 }; entry && written < k_MaxRecordsPerUpdate; ++written)
        {
            this->Write(*entry);
            entry = m_TickDumpUIStore.WaitPop(milliseconds{ 0 });
        }
        if (entry) this->Write(*entry);
    }

    auto TickDumpUIService::Shutdown() -> void
    {
        this->Finish();
    }

    auto TickDumpUIService::Draw() -> void
    {
        ImGui::SeparatorText("Tick dump");

        ImGui::TextWrapped("Records the Tick of every tick (the BridgeTick of Bridge.xml) to a .tdump and the Snapshot it came from to a"
            " .sdump, in TickDumps in the storage folder (D151, D202). Tools/tickdump.py reads the .tdump.");

        if (m_TickDumpUIStore.IsRecording())
        {
            if (ImGui::Button("Stop##TickDump")) m_TickDumpUIStore.RequestStop();
            if (m_TickDumpUIStore.IsWaitingForLoad())
            {
                ImGui::SameLine();
                ImGui::TextUnformatted("Waiting for the next map load.");
            }
        }
        else
        {
            if (ImGui::Button("Start##TickDump"))
            {
                m_TickDumpUIStore.SetWaitingForLoad(false);
                m_TickDumpUIStore.RequestStart("manual");
            }
            ImGui::SameLine();
            if (ImGui::Button("Start at next load##TickDump"))
            {
                m_TickDumpUIStore.SetWaitingForLoad(true);
                m_TickDumpUIStore.RequestStart("manual, from the next load");
            }
        }

        ImGui::SameLine();
        ImGui::Text("Ticks: %llu  .tdump: %.1f MB  .sdump: %.1f MB  Queued: %zu",
            static_cast<unsigned long long>(m_TickDumpUIStore.GetRecordedTicks()), ToMegabytes(m_TickDumpUIStore.GetWrittenBytes()),
            ToMegabytes(m_TickDumpUIStore.GetSnapshotBytes()), m_TickDumpUIStore.GetQueuedTicks());
        ImGui::Text("Dropped: %llu  Queue waits: %llu  Event losses: %llu",
            static_cast<unsigned long long>(m_TickDumpUIStore.GetDroppedTicks()),
            static_cast<unsigned long long>(m_TickDumpUIStore.GetQueueWaits()),
            static_cast<unsigned long long>(m_TickDumpUIStore.GetEventLosses()));

        const path filePath{ m_TickDumpUIStore.GetFilePath() };
        if (!filePath.empty()) ImGui::TextDisabled("%s", filePath.string().c_str());

        const std::string status{ m_TickDumpUIStore.GetStatus() };
        if (!status.empty()) ImGui::TextUnformatted(status.c_str());
    }

    auto TickDumpUIService::Start(std::string_view source) -> void
    {
        if (m_TickDumpUIStore.IsRecording()) return;

        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        if (directory.empty())
        {
            m_TickDumpUIStore.SetStatus("Storage Folder is not set (Settings).");
            return;
        }

        const path folder{ path(directory) / k_DumpFolder };
        std::error_code error{};
        std::filesystem::create_directories(folder, error);

        const std::string mapName{ path(m_FileStore.GetMapFilePath()).stem().string() };
        const auto now{ LocalNow() };
        const path filePath{ folder / std::format("{:%Y%m%d_%H%M%S}_{}.tdump", now, mapName.empty() ? "nomap" : mapName) };

        path snapshotPath{ filePath };
        snapshotPath.replace_extension(".sdump");

        std::ofstream& file{ m_TickDumpUIStore.GetFile() };
        file.open(filePath, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            m_TickDumpUIStore.SetStatus("The dump file cannot be opened.");
            m_Log.Message("[TickDumpUIService] ERROR: Failed to open the dump file.");
            return;
        }

        std::ofstream& snapshotFile{ m_TickDumpUIStore.GetSnapshotFile() };
        snapshotFile.open(snapshotPath, std::ios::binary | std::ios::trunc);
        if (!snapshotFile)
        {
            file.close();
            m_TickDumpUIStore.SetStatus("The snapshot dump file cannot be opened.");
            m_Log.Message("[TickDumpUIService] ERROR: Failed to open the snapshot dump file.");
            return;
        }

        const std::string startTime{ std::format("{:%Y-%m-%d %H:%M:%S}", now) };
        const std::uint64_t bytes{ Bridge::Codec::Dump::System::WriteFileHeader(file, mapName, startTime, source) };
        const std::uint64_t snapshotBytes{ Blam::Layout::Snapshot::System::WriteFileHeader(snapshotFile, k_BridgeLayoutHash, startTime, source) };

        m_TickDumpUIStore.ClearQueue();
        m_TickDumpUIStore.ResetCounters();
        m_TickDumpUIStore.AddWrittenBytes(bytes);
        m_TickDumpUIStore.AddSnapshotBytes(snapshotBytes);
        Bridge::Codec::Dump::System::Begin(m_TickDumpUIStore.GetWriterState());
        m_TickDumpUIStore.GetSnapshotWriterState() = Blam::Layout::Snapshot::Type::SnapshotWriterState{};
        m_TickDumpUIStore.SetFilePath(filePath);
        m_TickDumpUIStore.SetStatus({});
        m_TickDumpUIStore.SetRecording(true);

        m_Log.Message("[TickDumpUIService] INFO: Recording ticks to '{}' and '{}' ({}).", filePath.string(), snapshotPath.string(), source);
    }

    auto TickDumpUIService::Finish() -> void
    {
        if (!m_TickDumpUIStore.IsRecording()) return;

        // From here the AI thread pushes nothing more, so the queue only holds what it pushed before.
        m_TickDumpUIStore.SetRecording(false);
        while (auto entry{ m_TickDumpUIStore.WaitPop(milliseconds{ 0 }) }) this->Write(*entry);

        // A failed write in the drain already closed the files and reported it.
        std::ofstream& file{ m_TickDumpUIStore.GetFile() };
        std::ofstream& snapshotFile{ m_TickDumpUIStore.GetSnapshotFile() };
        if (!file.is_open() || !snapshotFile.is_open()) return;

        const TickDumpCounters counters{ .RecordedTicks = m_TickDumpUIStore.GetRecordedTicks(),
            .DroppedTicks = m_TickDumpUIStore.GetDroppedTicks(), .QueueWaits = m_TickDumpUIStore.GetQueueWaits(),
            .EventLosses = m_TickDumpUIStore.GetEventLosses() };
        m_TickDumpUIStore.AddWrittenBytes(Bridge::Codec::Dump::System::WriteFooter(file, counters));

        // The map loaded now. The CRC-32 takes about a second for a Reach map.
        SnapshotDumpFooter snapshotFooter{ DescribeMap(m_FileStore.GetMapFilePath(), true) };
        snapshotFooter.Marker = k_SnapshotDumpEndMarker;
        snapshotFooter.RecordedTicks = counters.RecordedTicks;
        m_TickDumpUIStore.AddSnapshotBytes(Blam::Layout::Snapshot::System::WriteFooter(snapshotFile, snapshotFooter));

        file.close();
        snapshotFile.close();
        if (file.fail() || snapshotFile.fail())
        {
            m_TickDumpUIStore.SetStatus("The footer could not be written: a file is cut.");
            m_Log.Message("[TickDumpUIService] ERROR: Failed to write the footer of the dump.");
            return;
        }

        const std::string status{ std::format("Saved {} ticks ({} dropped, {} queue waits, {} event losses).", counters.RecordedTicks,
            counters.DroppedTicks, counters.QueueWaits, counters.EventLosses) };
        m_TickDumpUIStore.SetStatus(status);
        m_Log.Message("[TickDumpUIService] INFO: Dump saved, {} ticks, {} bytes (.tdump), {} bytes (.sdump), {} dropped, {} queue waits, {} event losses.",
            counters.RecordedTicks, m_TickDumpUIStore.GetWrittenBytes(), m_TickDumpUIStore.GetSnapshotBytes(), counters.DroppedTicks,
            counters.QueueWaits, counters.EventLosses);
    }

    auto TickDumpUIService::Fail() -> void
    {
        m_TickDumpUIStore.SetRecording(false);
        m_TickDumpUIStore.ClearQueue();
        m_TickDumpUIStore.GetFile().close();
        m_TickDumpUIStore.GetSnapshotFile().close();

        m_TickDumpUIStore.SetStatus(std::format("Stopped: the file stopped taking writes after {} ticks (disk full?).",
            m_TickDumpUIStore.GetRecordedTicks()));
        m_Log.Message("[TickDumpUIService] ERROR: Failed to write the dump, recording stopped after {} ticks.",
            m_TickDumpUIStore.GetRecordedTicks());
    }

    auto TickDumpUIService::Write(const TickDumpEntry& entry) -> void
    {
        std::ofstream& file{ m_TickDumpUIStore.GetFile() };
        std::ofstream& snapshotFile{ m_TickDumpUIStore.GetSnapshotFile() };
        if (!file.is_open() || !snapshotFile.is_open() || entry.Tick == nullptr || entry.Snapshot == nullptr) return;

        // The offline test replays from an empty History, so a recording armed for the next load starts exactly there (D208).
        if (m_TickDumpUIStore.IsWaitingForLoad())
        {
            if (!entry.IsFirstAfterLoad) return;
            m_TickDumpUIStore.SetWaitingForLoad(false);
        }

        BridgeTick& flat{ m_TickDumpUIStore.GetFlat() };
        Bridge::Codec::Dump::System::ClearHeader(flat);
        m_FlatService.Flatten(*entry.Tick, flat);

        const auto result{ Bridge::Codec::Dump::System::WriteRecord(file, flat, entry.Tick->Match != nullptr, entry.DroppedBefore,
            m_TickDumpUIStore.GetWriterState()) };
        if (result.IsEventsLost) m_TickDumpUIStore.AddEventLoss();

        std::uint32_t flags{ 0 };
        std::optional<SnapshotLoadInfo> loadInfo{};
        if (entry.IsFirstAfterLoad)
        {
            flags |= k_SnapshotFirstAfterLoad;
            const SnapshotDumpFooter map{ DescribeMap(m_FileStore.GetMapFilePath(), false) };
            loadInfo = SnapshotLoadInfo{ .MapFileName = map.MapFileName, .MapFileSize = map.MapFileSize };
        }
        if (entry.IsViewerActive) flags |= k_SnapshotViewerActive;

        const std::uint64_t snapshotBytes{ Blam::Layout::Snapshot::System::WriteRecord(snapshotFile, entry.Snapshot, entry.Generation,
            entry.DroppedBefore, flags, loadInfo, m_TickDumpUIStore.GetSnapshotWriterState()) };

        if (!file.good() || !snapshotFile.good())
        {
            this->Fail();
            return;
        }

        m_TickDumpUIStore.AddDroppedTicks(entry.DroppedBefore);
        m_TickDumpUIStore.AddWrittenBytes(result.Bytes);
        m_TickDumpUIStore.AddSnapshotBytes(snapshotBytes);
        m_TickDumpUIStore.AddRecordedTick();
    }
}