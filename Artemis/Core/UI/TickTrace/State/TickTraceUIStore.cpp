module UI.TickTrace.State;

namespace UI::TickTrace::State
{
    auto TickTraceUIStore::SetRecording(bool value) -> void
    {
        std::lock_guard<std::mutex> lock{ m_PendingMutex };
        m_IsRecording = value;
    }

    auto TickTraceUIStore::IsRecording() const -> bool
    {
        std::lock_guard<std::mutex> lock{ m_PendingMutex };
        return m_IsRecording;
    }

    auto TickTraceUIStore::SetPendingClock(const TickClock& clock) -> void
    {
        m_PendingClock = clock;
    }

    auto TickTraceUIStore::GetPendingClock() const -> TickClock
    {
        return m_PendingClock;
    }

    auto TickTraceUIStore::Append(std::span<const std::byte> bytes) -> bool
    {
        std::lock_guard<std::mutex> lock{ m_PendingMutex };
        if (!m_IsRecording) return false;

        m_Pending.insert(m_Pending.end(), bytes.begin(), bytes.end());
        return true;
    }

    auto TickTraceUIStore::TakePending(std::vector<std::byte>& out) -> void
    {
        out.clear();

        std::lock_guard<std::mutex> lock{ m_PendingMutex };
        out.swap(m_Pending);
    }

    auto TickTraceUIStore::AddRecordedTick() -> void
    {
        m_RecordedTicks.fetch_add(1, std::memory_order_relaxed);
    }

    auto TickTraceUIStore::GetRecordedTicks() const -> std::uint64_t
    {
        return m_RecordedTicks.load(std::memory_order_relaxed);
    }

    auto TickTraceUIStore::AddWrittenBytes(std::uint64_t bytes) -> void
    {
        m_WrittenBytes.fetch_add(bytes, std::memory_order_relaxed);
    }

    auto TickTraceUIStore::GetWrittenBytes() const -> std::uint64_t
    {
        return m_WrittenBytes.load(std::memory_order_relaxed);
    }

    auto TickTraceUIStore::SetStartTime(steady_clock::time_point time) -> void
    {
        m_StartTime = time;
    }

    auto TickTraceUIStore::GetStartTime() const -> steady_clock::time_point
    {
        return m_StartTime;
    }

    auto TickTraceUIStore::GetFile() -> std::ofstream&
    {
        return m_File;
    }

    auto TickTraceUIStore::SetFilePath(path filePath) -> void
    {
        m_FilePath = std::move(filePath);
    }

    auto TickTraceUIStore::GetFilePath() const -> const path&
    {
        return m_FilePath;
    }

    auto TickTraceUIStore::SetStatus(std::string status) -> void
    {
        m_Status = std::move(status);
    }

    auto TickTraceUIStore::GetStatus() const -> const std::string&
    {
        return m_Status;
    }

    auto TickTraceUIStore::ResetCounters() -> void
    {
        m_RecordedTicks.store(0, std::memory_order_relaxed);
        m_WrittenBytes.store(0, std::memory_order_relaxed);
    }
}