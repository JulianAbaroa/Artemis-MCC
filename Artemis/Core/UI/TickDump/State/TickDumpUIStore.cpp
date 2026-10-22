module UI.TickDump.State;

import std;

namespace UI::TickDump::State
{
    auto TickDumpUIStore::RequestStart(std::string source) -> void
    {
        std::lock_guard lock{ m_TextMutex };
        m_StartRequest = std::move(source);
    }

    auto TickDumpUIStore::TakeStartRequest() -> std::optional<std::string>
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
        {
            std::lock_guard lock{ m_QueueMutex };
            m_IsRecording.store(value, std::memory_order_release);
        }
        m_NotFull.notify_all();
    }

    auto TickDumpUIStore::IsRecording() const -> bool
    {
        return m_IsRecording.load(std::memory_order_acquire);
    }

    auto TickDumpUIStore::Push(TickDumpEntry entry) -> void
    {
        {
            std::unique_lock lock{ m_QueueMutex };
            if (m_Queue.size() >= k_QueueCapacity)
            {
                m_QueueWaits.fetch_add(1, std::memory_order_relaxed);
                m_NotFull.wait(lock, [this] {
                    return m_Queue.size() < k_QueueCapacity || !m_IsRecording.load(std::memory_order_acquire);
                });
            }
            if (!m_IsRecording.load(std::memory_order_acquire)) return;
            m_Queue.push_back(std::move(entry));
        }
        m_NotEmpty.notify_one();
    }

    auto TickDumpUIStore::WaitPop(milliseconds timeout) -> std::optional<TickDumpEntry>
    {
        std::optional<TickDumpEntry> entry{};
        {
            std::unique_lock lock{ m_QueueMutex };
            if (!m_NotEmpty.wait_for(lock, timeout, [this] { return !m_Queue.empty(); })) return std::nullopt;
            entry = std::move(m_Queue.front());
            m_Queue.pop_front();
        }
        m_NotFull.notify_one();
        return entry;
    }

    auto TickDumpUIStore::GetQueuedTicks() const -> std::size_t
    {
        std::lock_guard lock{ m_QueueMutex };
        return m_Queue.size();
    }

    auto TickDumpUIStore::ClearQueue() -> void
    {
        {
            std::lock_guard lock{ m_QueueMutex };
            m_Queue.clear();
        }
        m_NotFull.notify_all();
    }

    auto TickDumpUIStore::GetFlat() -> BridgeTick&
    {
        return *m_Flat;
    }

    auto TickDumpUIStore::GetPreviousEntities() -> std::vector<BridgeEntity>&
    {
        return m_PreviousEntities;
    }

    auto TickDumpUIStore::GetRecordsSinceKeyframe() const -> std::uint32_t
    {
        return m_RecordsSinceKeyframe;
    }

    auto TickDumpUIStore::SetRecordsSinceKeyframe(std::uint32_t records) -> void
    {
        m_RecordsSinceKeyframe = records;
    }

    auto TickDumpUIStore::GetLastEventSequence() const -> std::uint32_t
    {
        return m_LastEventSequence;
    }

    auto TickDumpUIStore::SetLastEventSequence(std::uint32_t sequence) -> void
    {
        m_LastEventSequence = sequence;
    }

    auto TickDumpUIStore::GetFile() -> std::ofstream&
    {
        return m_File;
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
        m_DroppedTicks.store(0, std::memory_order_relaxed);
        m_QueueWaits.store(0, std::memory_order_relaxed);
        m_EventLosses.store(0, std::memory_order_relaxed);
    }
}