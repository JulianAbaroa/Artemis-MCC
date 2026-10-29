module;

#include "External/imgui/imgui.h"

module DevTools.Workbench.TickTrace.System;

import Common.Math.Type;
import Common.Text.System;
import Common.Time.System;
import Blam.Layout.Object.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Object.Type;
import std;

namespace
{
    using std::chrono::steady_clock;
    using std::chrono::nanoseconds;
    using std::filesystem::path;
    using Common::Text::System::CopyText;
    using Common::Time::System::LocalNow;

    using Blam::Layout::Snapshot::Type::RegionId;
    using Blam::Layout::Snapshot::Type::Snapshot;

    using DevTools::Workbench::TickTrace::Type::TraceFileHeader;
    using DevTools::Workbench::TickTrace::Type::TickRecord;
    using DevTools::Workbench::TickTrace::Type::ObjectRecord;

    using Common::Text::System::k_BytesPerMegabyte;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsEngineTickOffset;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsScaleOffset;
    using Blam::Layout::Object::Type::Constant::k_TimeGlobalsTickFractionOffset;
    using Blam::Layout::Object::Type::Constant::k_EventGlobalsSequenceOffset;

    // Subfolder of the storage folder that holds the traces.
    constexpr std::string_view k_TraceFolder{ "TickTraces" };

    // TickRecord::Flags bit set when the clock fields are valid.
    constexpr std::uint32_t k_ClockValidFlag{ 1 };

    // Object groups written to the trace.
    constexpr std::array<std::string_view, 4> k_TracedGroups{ "bipd", "vehi", "proj", "weap" };

    // Writes size bytes starting at data to the trace file.
    auto WriteBytes(std::ofstream& file, const void* data, std::size_t size) -> void
    {
        file.write(std::bit_cast<const char*>(data), static_cast<std::streamsize>(size));
    }

    auto IsTraced(std::string_view fourCC) -> bool
    {
        return std::ranges::find(k_TracedGroups, fourCC) != k_TracedGroups.end();
    }
}

namespace DevTools::Workbench::TickTrace::System
{
    auto TickTraceUIService::CaptureClock(const Snapshot& snapshot) -> void
    {
        if (!m_TickTraceUIStore.IsRecording()) return;

        m_TickTraceUIStore.SetPendingClock(this->ReadClock(snapshot));
    }

    auto TickTraceUIService::Record(std::uint64_t generation) -> void
    {
        if (!m_TickTraceUIStore.IsRecording()) return;

        std::vector<ObjectRecord> objects{};

        if (const auto table{ m_ObjectStore.Acquire() })
        {
            objects.reserve(table->size());

            for (const auto& [handle, object] : *table)
            {
                if (!IsTraced(object.FourCC)) continue;

                ObjectRecord record{};
                record.Handle = object.Handle;
                record.ParentHandle = object.ParentHandle;
                std::copy_n(object.FourCC.data(), (std::min)(object.FourCC.size(), record.FourCC.size()), record.FourCC.begin());
                record.DamageReceived = object.DamageReceived;
                record.BoundingCenter = object.BoundingCenter;
                record.WorldOrigin = object.WorldOrigin;
                record.Forward = object.Forward;
                record.Up = object.Up;
                record.LinearVelocity = object.LinearVelocity;
                record.AngularVelocity = object.AngularVelocity;

                objects.push_back(record);
            }
        }

        // The table is a hash map, so the order is sorted here to compare traces object by object.
        std::ranges::sort(objects, {}, &ObjectRecord::Handle);

        const TickClock clock{ m_TickTraceUIStore.GetPendingClock() };

        TickRecord tick{};
        tick.Generation = generation;
        tick.RealTimeNs = static_cast<std::uint64_t>(
            std::chrono::duration_cast<nanoseconds>(steady_clock::now() - m_TickTraceUIStore.GetStartTime()).count());
        tick.EngineTick = clock.EngineTick;
        tick.TimeScale = clock.TimeScale;
        tick.TickFraction = clock.TickFraction;
        tick.EventSequence = clock.EventSequence;
        tick.ObjectCount = static_cast<std::uint32_t>(objects.size());
        tick.Flags = clock.IsValid ? k_ClockValidFlag : 0;

        std::vector<std::byte> bytes(sizeof(TickRecord) + objects.size() * sizeof(ObjectRecord));
        std::memcpy(bytes.data(), &tick, sizeof(TickRecord));

        if (!objects.empty())
        {
            std::memcpy(bytes.data() + sizeof(TickRecord), objects.data(), objects.size() * sizeof(ObjectRecord));
        }

        if (m_TickTraceUIStore.Append(bytes)) m_TickTraceUIStore.AddRecordedTick();
    }

    auto TickTraceUIService::Update() -> void
    {
        if (m_TickTraceUIStore.TakeStartRequest()) this->Start();
        if (m_TickTraceUIStore.TakeStopRequest())
        {
            this->Stop();
            return;
        }

        std::vector<std::byte> pending{};
        m_TickTraceUIStore.TakePending(pending);
        if (pending.empty()) return;

        std::ofstream& file{ m_TickTraceUIStore.GetFile() };
        if (!file.is_open()) return;

        WriteBytes(file, pending.data(), pending.size());
        m_TickTraceUIStore.AddWrittenBytes(pending.size());
    }

    auto TickTraceUIService::Stop() -> void
    {
        if (!m_TickTraceUIStore.IsRecording()) return;

        // No record is appended after this, so the last Update writes everything.
        m_TickTraceUIStore.SetRecording(false);
        this->Update();

        m_TickTraceUIStore.GetFile().close();

        const std::uint64_t ticks{ m_TickTraceUIStore.GetRecordedTicks() };
        m_TickTraceUIStore.SetStatus(std::format("Saved {} ticks.", ticks));

        m_Log.Message("[TickTraceUIService] INFO: Trace saved, {} ticks, {} bytes.",
            ticks, m_TickTraceUIStore.GetWrittenBytes());
    }

    auto TickTraceUIService::Draw() -> void
    {
        ImGui::SeparatorText("Tick trace");

        ImGui::TextWrapped("Records every tick (engine clock, bipeds, vehicles, projectiles and weapons) to TickTraces in the storage"
            " folder. Tools/analyze_ticktrace.py compares two traces tick by tick.");

        if (m_TickTraceUIStore.IsRecording())
        {
            if (ImGui::Button("Stop")) this->Stop();
        }
        else
        {
            if (ImGui::Button("Start")) this->Start();
        }

        ImGui::SameLine();
        ImGui::Text("Ticks: %llu  Size: %.1f MB", static_cast<unsigned long long>(m_TickTraceUIStore.GetRecordedTicks()),
            static_cast<double>(m_TickTraceUIStore.GetWrittenBytes()) / k_BytesPerMegabyte);

        const path& filePath{ m_TickTraceUIStore.GetFilePath() };
        if (!filePath.empty()) ImGui::TextDisabled("%s", filePath.string().c_str());

        const std::string& status{ m_TickTraceUIStore.GetStatus() };
        if (!status.empty()) ImGui::TextUnformatted(status.c_str());
    }

    auto TickTraceUIService::Start() -> void
    {
        if (m_TickTraceUIStore.IsRecording()) return;

        const std::string directory{ m_SettingsStore.GetAppDataDirectory() };
        if (directory.empty())
        {
            m_TickTraceUIStore.SetStatus("Storage Folder is not set (Settings).");
            return;
        }

        const path folder{ path(directory) / k_TraceFolder };
        std::error_code error{};
        std::filesystem::create_directories(folder, error);

        const std::string mapName{ path(m_FileStore.GetMapFilePath()).stem().string() };
        const auto now{ LocalNow() };
        const path filePath{ folder / std::format("{:%Y%m%d_%H%M%S}_{}.attrace", now, mapName.empty() ? "nomap" : mapName) };

        std::ofstream& file{ m_TickTraceUIStore.GetFile() };
        file.open(filePath, std::ios::binary | std::ios::trunc);

        if (!file)
        {
            m_TickTraceUIStore.SetStatus("The trace file cannot be opened.");
            m_Log.Message("[TickTraceUIService] ERROR: Failed to open the trace file.");
            return;
        }

        TraceFileHeader header{};
        header.TickRecordSize = sizeof(TickRecord);
        header.ObjectRecordSize = sizeof(ObjectRecord);
        CopyText(mapName, header.MapName);
        CopyText(std::format("{:%Y-%m-%d %H:%M:%S}", now), header.StartTime);

        WriteBytes(file, &header, sizeof(header));

        // Drops records a previous recording left behind.
        std::vector<std::byte> stale{};
        m_TickTraceUIStore.TakePending(stale);

        m_TickTraceUIStore.ResetCounters();
        m_TickTraceUIStore.AddWrittenBytes(sizeof(header));
        m_TickTraceUIStore.SetFilePath(filePath);
        m_TickTraceUIStore.SetStatus({});
        m_TickTraceUIStore.SetStartTime(steady_clock::now());
        m_TickTraceUIStore.SetRecording(true);

        m_Log.Message("[TickTraceUIService] INFO: Recording ticks to '{}'.", filePath.string());
    }

    auto TickTraceUIService::ReadClock(const Snapshot& snapshot) const -> TickClock
    {
        TickClock clock{};

        if (!snapshot.HasThreadContext || !snapshot.IsCaptured(RegionId::TimeGlobals)) return clock;

        clock.EngineTick = snapshot.Read<std::uint32_t>(RegionId::TimeGlobals, k_TimeGlobalsEngineTickOffset);
        clock.TimeScale = snapshot.Read<float>(RegionId::TimeGlobals, k_TimeGlobalsScaleOffset);
        clock.TickFraction = snapshot.Read<float>(RegionId::TimeGlobals, k_TimeGlobalsTickFractionOffset);

        if (snapshot.IsCaptured(RegionId::EventGlobals))
        {
            clock.EventSequence = snapshot.Read<std::uint32_t>(RegionId::EventGlobals, k_EventGlobalsSequenceOffset);
        }

        clock.IsValid = true;
        return clock;
    }
}