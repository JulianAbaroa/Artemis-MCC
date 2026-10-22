module;

#include "External/imgui/imgui.h"

module UI.TickDump.System;

import Common.Text.System;
import Common.Time.System;
import Platform.Bridge.Type;
import std;

namespace
{
    using std::filesystem::path;
    using Common::Text::System::CopyText;
    using Common::Time::System::LocalNow;

    using Platform::Bridge::Type::BridgeTick;
    using Platform::Bridge::Type::BridgeTickHeader;
    using Platform::Bridge::Type::BridgeEvent;
    using Platform::Bridge::Type::BridgePlayer;
    using Platform::Bridge::Type::BridgeEntity;

    using UI::TickDump::Type::TickDumpFileHeader;
    using UI::TickDump::Type::TickRecordHeader;
    using UI::TickDump::Type::TickDumpFooter;

    using Common::Text::System::k_BytesPerMegabyte;
    using Platform::Bridge::Type::k_BridgeLayoutJson;
    using Platform::Bridge::Type::k_BridgeLayoutHash;
    using Platform::Bridge::Type::k_BridgeLayoutVersion;
    using UI::TickDump::Type::k_TickDumpVersion;
    using UI::TickDump::Type::k_TickDumpMagic;
    using UI::TickDump::Type::k_TickRecordMarker;
    using UI::TickDump::Type::k_TickDumpEndMarker;
    using UI::TickDump::Type::k_TickRecordHasMatch;
    using UI::TickDump::Type::k_TickRecordEventsLost;
    using UI::TickDump::Type::k_TickRecordKeyframe;

    // Subfolder of the storage folder that holds the dumps.
    constexpr std::string_view k_DumpFolder{ "TickDumps" };

    // Records between two keyframes: 10 s of game. A reader rebuilds a tick from the keyframe before it.
    constexpr std::uint32_t k_KeyframeInterval{ 600 };

    // Records written by one Update at most, so the requests of the UI are seen even when the queue never empties.
    constexpr std::size_t k_MaxRecordsPerUpdate{ 600 };

    // return: Bytes written.
    template <typename T>
    auto WriteValue(std::ofstream& file, const T& value) -> std::uint64_t
    {
        file.write(reinterpret_cast<const char*>(&value), sizeof(T));
        return sizeof(T);
    }

    // return: Bytes written.
    template <typename T>
    auto WriteRows(std::ofstream& file, std::span<const T> rows) -> std::uint64_t
    {
        if (rows.empty()) return 0;
        file.write(reinterpret_cast<const char*>(rows.data()), static_cast<std::streamsize>(rows.size_bytes()));
        return rows.size_bytes();
    }
}

namespace UI::TickDump::System
{
    auto TickDumpUIService::IsRecording() const -> bool
    {
        return m_TickDumpUIStore.IsRecording();
    }

    auto TickDumpUIService::Push(std::shared_ptr<const Tick> snapshot, std::uint64_t droppedBefore) -> void
    {
        if (!m_TickDumpUIStore.IsRecording() || snapshot == nullptr) return;

        m_TickDumpUIStore.Push(TickDumpEntry{ .Snapshot = std::move(snapshot),
            .DroppedBefore = static_cast<std::uint32_t>((std::min)(droppedBefore, std::uint64_t{ (std::numeric_limits<std::uint32_t>::max)() })) });
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

        ImGui::TextWrapped("Records the Tick of every tick (the BridgeTick of Bridge.xml) to TickDumps in the storage folder (D151)."
            " Tools/tickdump.py reads the files.");

        if (m_TickDumpUIStore.IsRecording())
        {
            if (ImGui::Button("Stop##TickDump")) m_TickDumpUIStore.RequestStop();
        }
        else
        {
            if (ImGui::Button("Start##TickDump")) m_TickDumpUIStore.RequestStart("manual");
        }

        ImGui::SameLine();
        ImGui::Text("Ticks: %llu  Size: %.1f MB  Queued: %zu", static_cast<unsigned long long>(m_TickDumpUIStore.GetRecordedTicks()),
            static_cast<double>(m_TickDumpUIStore.GetWrittenBytes()) / k_BytesPerMegabyte, m_TickDumpUIStore.GetQueuedTicks());
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

        std::ofstream& file{ m_TickDumpUIStore.GetFile() };
        file.open(filePath, std::ios::binary | std::ios::trunc);
        if (!file)
        {
            m_TickDumpUIStore.SetStatus("The dump file cannot be opened.");
            m_LogsService.Message("[TickDumpUIService] ERROR: Failed to open the dump file.");
            return;
        }

        TickDumpFileHeader header{ .Magic = k_TickDumpMagic, .Version = k_TickDumpVersion,
            .HeaderSize = static_cast<std::uint32_t>(sizeof(TickDumpFileHeader)), .LayoutHash = k_BridgeLayoutHash,
            .LayoutVersion = k_BridgeLayoutVersion, .LayoutJsonSize = static_cast<std::uint32_t>(k_BridgeLayoutJson.size()),
            .RecordHeaderSize = static_cast<std::uint32_t>(sizeof(TickRecordHeader)) };
        CopyText(mapName, header.MapName);
        CopyText(std::format("{:%Y-%m-%d %H:%M:%S}", now), header.StartTime);
        CopyText(source, header.Source);

        std::uint64_t bytes{ WriteValue(file, header) };
        file.write(k_BridgeLayoutJson.data(), static_cast<std::streamsize>(k_BridgeLayoutJson.size()));
        bytes += k_BridgeLayoutJson.size();

        m_TickDumpUIStore.ClearQueue();
        m_TickDumpUIStore.ResetCounters();
        m_TickDumpUIStore.AddWrittenBytes(bytes);
        m_TickDumpUIStore.SetLastEventSequence(0);
        m_TickDumpUIStore.GetPreviousEntities().clear();
        m_TickDumpUIStore.SetRecordsSinceKeyframe(k_KeyframeInterval);
        m_TickDumpUIStore.SetFilePath(filePath);
        m_TickDumpUIStore.SetStatus({});
        m_TickDumpUIStore.SetRecording(true);

        m_LogsService.Message("[TickDumpUIService] INFO: Recording ticks to '{}' ({}).", filePath.string(), source);
    }

    auto TickDumpUIService::Finish() -> void
    {
        if (!m_TickDumpUIStore.IsRecording()) return;

        // From here the AI thread pushes nothing more, so the queue only holds what it pushed before.
        m_TickDumpUIStore.SetRecording(false);
        while (auto entry{ m_TickDumpUIStore.WaitPop(milliseconds{ 0 }) }) this->Write(*entry);

        // A failed write in the drain already closed the file and reported it.
        std::ofstream& file{ m_TickDumpUIStore.GetFile() };
        if (!file.is_open()) return;
        const TickDumpFooter footer{ .Marker = k_TickDumpEndMarker, .RecordedTicks = m_TickDumpUIStore.GetRecordedTicks(),
            .DroppedTicks = m_TickDumpUIStore.GetDroppedTicks(), .QueueWaits = m_TickDumpUIStore.GetQueueWaits(),
            .EventLosses = m_TickDumpUIStore.GetEventLosses() };
        m_TickDumpUIStore.AddWrittenBytes(WriteValue(file, footer));
        file.close();
        if (file.fail())
        {
            m_TickDumpUIStore.SetStatus("The footer could not be written: the file is cut.");
            m_LogsService.Message("[TickDumpUIService] ERROR: Failed to write the footer of the dump.");
            return;
        }

        const std::string status{ std::format("Saved {} ticks ({} dropped, {} queue waits, {} event losses).", footer.RecordedTicks,
            footer.DroppedTicks, footer.QueueWaits, footer.EventLosses) };
        m_TickDumpUIStore.SetStatus(status);
        m_LogsService.Message("[TickDumpUIService] INFO: Dump saved, {} ticks, {} bytes, {} dropped, {} queue waits, {} event losses.",
            footer.RecordedTicks, m_TickDumpUIStore.GetWrittenBytes(), footer.DroppedTicks, footer.QueueWaits, footer.EventLosses);
    }

    auto TickDumpUIService::Fail() -> void
    {
        m_TickDumpUIStore.SetRecording(false);
        m_TickDumpUIStore.ClearQueue();
        m_TickDumpUIStore.GetFile().close();

        m_TickDumpUIStore.SetStatus(std::format("Stopped: the file stopped taking writes after {} ticks (disk full?).",
            m_TickDumpUIStore.GetRecordedTicks()));
        m_LogsService.Message("[TickDumpUIService] ERROR: Failed to write the dump, recording stopped after {} ticks.",
            m_TickDumpUIStore.GetRecordedTicks());
    }

    auto TickDumpUIService::Write(const TickDumpEntry& entry) -> void
    {
        std::ofstream& file{ m_TickDumpUIStore.GetFile() };
        if (!file.is_open() || entry.Snapshot == nullptr) return;

        // The clock fields of the header belong to the bridge (BridgeService::EndTick): zero in a dump.
        BridgeTick& flat{ m_TickDumpUIStore.GetFlat() };
        flat.Header = BridgeTickHeader{};
        m_FlatService.Flatten(*entry.Snapshot, flat);

        TickRecordHeader record{ .Marker = k_TickRecordMarker, .DroppedBefore = entry.DroppedBefore };
        if (entry.Snapshot->Match)
        {
            record.GameTick = entry.Snapshot->Match->GameTick;
            record.Flags |= k_TickRecordHasMatch;
        }

        // The BridgeTick carries the newest events of the match (oldest first, Sequence from 1 in each match), so each record
        // writes only the ones not written yet. A smaller LastEventSequence means a new match.
        std::uint32_t last{ m_TickDumpUIStore.GetLastEventSequence() };
        if (flat.Header.LastEventSequence < last) last = 0;
        const std::span<const BridgeEvent> window{ flat.Events.data(), flat.Header.EventCount };
        std::size_t first{ 0 };
        while (first < window.size() && window[first].Sequence <= last) ++first;
        const std::span<const BridgeEvent> fresh{ window.subspan(first) };
        if (last != 0 && !fresh.empty() && fresh.front().Sequence != last + 1)
        {
            record.Flags |= k_TickRecordEventsLost;
            m_TickDumpUIStore.AddEventLoss();
        }
        record.NewEventCount = static_cast<std::uint16_t>(fresh.size());

        // Entities against the previous record. Both are sorted by handle (Environment Entities), so one merge finds the rows that
        // are new or changed and the handles that are gone.
        const std::span<const BridgeEntity> current{ flat.Entities.data(), flat.Header.EntityCount };
        std::vector<BridgeEntity>& previous{ m_TickDumpUIStore.GetPreviousEntities() };
        const bool isKeyframe{ m_TickDumpUIStore.GetRecordsSinceKeyframe() >= k_KeyframeInterval };
        std::vector<BridgeEntity> changed{};
        std::vector<std::uint32_t> removed{};
        if (isKeyframe)
        {
            record.Flags |= k_TickRecordKeyframe;
            changed.assign(current.begin(), current.end());
        }
        else
        {
            std::size_t i{ 0 };
            std::size_t j{ 0 };
            while (i < previous.size() || j < current.size())
            {
                if (j == current.size() || (i < previous.size() && previous[i].Handle < current[j].Handle))
                {
                    removed.push_back(previous[i++].Handle);
                }
                else if (i == previous.size() || current[j].Handle < previous[i].Handle)
                {
                    changed.push_back(current[j++]);
                }
                else
                {
                    if (std::memcmp(&previous[i], &current[j], sizeof(BridgeEntity)) != 0) changed.push_back(current[j]);
                    ++i;
                    ++j;
                }
            }
        }
        record.ChangedEntityCount = static_cast<std::uint16_t>(changed.size());
        record.RemovedEntityCount = static_cast<std::uint16_t>(removed.size());

        std::uint64_t bytes{ WriteValue(file, record) };
        bytes += WriteValue(file, flat.Header);
        bytes += WriteValue(file, flat.Match);
        bytes += WriteRows(file, std::span<const BridgePlayer>{ flat.Players.data(), flat.Header.PlayerCount });
        bytes += WriteRows(file, std::span<const BridgeEntity>{ changed });
        bytes += WriteRows(file, std::span<const std::uint32_t>{ removed });
        bytes += WriteRows(file, fresh);

        if (!file.good())
        {
            this->Fail();
            return;
        }

        previous.assign(current.begin(), current.end());
        m_TickDumpUIStore.SetRecordsSinceKeyframe(isKeyframe ? 1 : m_TickDumpUIStore.GetRecordsSinceKeyframe() + 1);
        m_TickDumpUIStore.SetLastEventSequence(flat.Header.LastEventSequence);
        m_TickDumpUIStore.AddDroppedTicks(entry.DroppedBefore);
        m_TickDumpUIStore.AddWrittenBytes(bytes);
        m_TickDumpUIStore.AddRecordedTick();
    }
}