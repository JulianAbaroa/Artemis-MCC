module DevTools.Workbench.TickDump.State;

import std;

namespace DevTools::Workbench::TickDump::State
{
    auto TickDumpUIStore::RequestStart(std::string source, bool isSnapshotWritten) -> void
    {
        std::lock_guard lock{ m_TextMutex };
        m_StartRequest = TickDumpStartRequest{ .Source = std::move(source), .IsSnapshotWritten = isSnapshotWritten };
    }

    auto TickDumpUIStore::TakeStartRequest() -> std::optional<TickDumpStartRequest>
    {
        std::lock_guard lock{ m_TextMutex };
        return std::exchange(m_StartRequest, std::nullopt);
    }

    auto TickDumpUIStore::RequestStop() -> void
    {
        m_IsStopRequested.store(true, std::memory_order_release);
    }

    auto TickDumpUIStore::TakeStopRequest() -> bool
    {
        return m_IsStopRequested.exchange(false, std::memory_order_acq_rel);
    }

    auto TickDumpUIStore::SetRecording(bool value) -> void
    {
        m_IsRecording.store(value, std::memory_order_release);
        m_Queue.SetOpen(value);
    }

    auto TickDumpUIStore::SetSnapshotWritten(bool value) -> void
    {
        m_IsSnapshotWritten = value;
    }

    auto TickDumpUIStore::IsSnapshotWritten() const -> bool
    {
        return m_IsSnapshotWritten;
    }

    auto TickDumpUIStore::SetLastRecordAt(time_point value) -> void
    {
        m_LastRecordAt = value;
    }

    auto TickDumpUIStore::GetLastRecordAt() const -> time_point
    {
        return m_LastRecordAt;
    }

    auto TickDumpUIStore::SetWaitingForLoad(bool value) -> void
    {
        m_IsWaitingForLoad.store(value, std::memory_order_release);
    }

    auto TickDumpUIStore::IsWaitingForLoad() const -> bool
    {
        return m_IsWaitingForLoad.load(std::memory_order_acquire);
    }

    auto TickDumpUIStore::IsRecording() const -> bool
    {
        return m_IsRecording.load(std::memory_order_acquire);
    }

    auto TickDumpUIStore::EstimateBytes(const TickDumpEntry& entry) const -> std::size_t
    {
        std::size_t bytes{ sizeof(TickDumpEntry) };
        if (entry.Tick != nullptr) bytes += sizeof(*entry.Tick);
        if (entry.Snapshot != nullptr)
        {
            bytes += sizeof(*entry.Snapshot) + entry.Snapshot->RawObjects.CapacityBytes();
            for (const auto& region : entry.Snapshot->Regions) bytes += region.Bytes.capacity();
        }
        return bytes;
    }

    auto TickDumpUIStore::Push(TickDumpEntry entry) -> void
    {
        const std::size_t bytes{ this->EstimateBytes(entry) };
        if (m_Queue.Push(std::move(entry), bytes).IsWaited) m_QueueWaits.fetch_add(1, std::memory_order_relaxed);
    }

    auto TickDumpUIStore::WaitPop(milliseconds timeout) -> std::optional<TickDumpEntry>
    {
        return m_Queue.WaitPop(timeout);
    }

    auto TickDumpUIStore::GetQueuedTicks() const -> std::size_t
    {
        return m_Queue.Size();
    }

    auto TickDumpUIStore::ClearQueue() -> void
    {
        m_Queue.Clear();
    }

    auto TickDumpUIStore::GetFlat() -> BridgeTick&
    {
        return *m_Flat;
    }

    auto TickDumpUIStore::GetWriterState() -> TickDumpWriterState&
    {
        return m_WriterState;
    }

    auto TickDumpUIStore::GetSnapshotWriterState() -> SnapshotWriterState&
    {
        return m_SnapshotWriterState;
    }

    auto TickDumpUIStore::GetFile() -> std::ofstream&
    {
        return m_File;
    }

    auto TickDumpUIStore::GetSnapshotFile() -> std::ofstream&
    {
        return m_SnapshotFile;
    }

    auto TickDumpUIStore::SetFilePath(path filePath) -> void
    {
        std::lock_guard lock{ m_TextMutex };
        m_FilePath = std::move(filePath);
    }

    auto TickDumpUIStore::GetFilePath() const -> path
    {
        std::lock_guard lock{ m_TextMutex };
        return m_FilePath;
    }

    auto TickDumpUIStore::SetStatus(std::string status) -> void
    {
        std::lock_guard lock{ m_TextMutex };
        m_Status = std::move(status);
    }

    auto TickDumpUIStore::GetStatus() const -> std::string
    {
        std::lock_guard lock{ m_TextMutex };
        return m_Status;
    }

    auto TickDumpUIStore::AddRecordedTick() -> void
    {
        m_RecordedTicks.fetch_add(1, std::memory_order_relaxed);
    }

    auto TickDumpUIStore::GetRecordedTicks() const -> std::uint64_t
    {
        return m_RecordedTicks.load(std::memory_order_relaxed);
    }

    auto TickDumpUIStore::AddWrittenBytes(std::uint64_t bytes) -> void
    {
        m_WrittenBytes.fetch_add(bytes, std::memory_order_relaxed);
    }

    auto TickDumpUIStore::GetWrittenBytes() const -> std::uint64_t
    {
        return m_WrittenBytes.load(std::memory_order_relaxed);
    }

    auto TickDumpUIStore::AddSnapshotBytes(std::uint64_t bytes) -> void
    {
        m_SnapshotBytes.fetch_add(bytes, std::memory_order_relaxed);
    }

    auto TickDumpUIStore::GetSnapshotBytes() const -> std::uint64_t
    {
        return m_SnapshotBytes.load(std::memory_order_relaxed);
    }

    auto TickDumpUIStore::AddDroppedTicks(std::uint64_t ticks) -> void
    {
        m_DroppedTicks.fetch_add(ticks, std::memory_order_relaxed);
    }

    auto TickDumpUIStore::GetDroppedTicks() const -> std::uint64_t
    {
        return m_DroppedTicks.load(std::memory_order_relaxed);
    }

    auto TickDumpUIStore::GetQueueWaits() const -> std::uint64_t
    {
        return m_QueueWaits.load(std::memory_order_relaxed);
    }

    auto TickDumpUIStore::AddEventLoss() -> void
    {
        m_EventLosses.fetch_add(1, std::memory_order_relaxed);
    }

    auto TickDumpUIStore::GetEventLosses() const -> std::uint64_t
    {
        return m_EventLosses.load(std::memory_order_relaxed);
    }

    auto TickDumpUIStore::ResetCounters() -> void
    {
        m_RecordedTicks.store(0, std::memory_order_relaxed);
        m_WrittenBytes.store(0, std::memory_order_relaxed);
        m_SnapshotBytes.store(0, std::memory_order_relaxed);
        m_DroppedTicks.store(0, std::memory_order_relaxed);
        m_QueueWaits.store(0, std::memory_order_relaxed);
        m_EventLosses.store(0, std::memory_order_relaxed);
    }
}