export module Common.State.Type;

import std;

export namespace Common::State::Type
{
    // Holds the latest published value and hands it out as an immutable snapshot.
    // The writer publishes a new value and the readers keep using the snapshot they acquired.
    template <typename T>
    class Snapshot
    {
    public:
        Snapshot() = default;
        ~Snapshot() = default;

        Snapshot(const Snapshot&) = delete;
        auto operator=(const Snapshot&) -> Snapshot& = delete;

        // Replaces the current snapshot with the new value.
        auto Publish(T value) -> void
        {
            auto snap = std::make_shared<const T>(std::move(value));
            m_Value.store(snap, std::memory_order_release);
        }

        // return: The current snapshot, or null if nothing was published or it was cleaned up.
        auto Acquire() const -> std::shared_ptr<const T>
        {
            return m_Value.load(std::memory_order_acquire);
        }

        // Drops the current snapshot. Readers that already acquired it keep their copy.
        auto Cleanup() -> void
        {
            m_Value.store(nullptr, std::memory_order_release);
        }

    private:
        std::atomic<std::shared_ptr<const T>> m_Value{};
    };

    // A FIFO queue between two threads bounded by the bytes its entries keep alive, not by their count.
    // The producer waits while the next entry does not fit, so nothing is lost. An entry alone larger than the cap still goes in
    // (the queue is empty then), so the producer never hangs. The queue starts closed: Push does nothing until SetOpen(true).
    template <typename T>
    class ByteBoundedQueue
    {
    public:
        struct PushResult
        {
            // The entry was queued (false if the queue was closed).
            bool IsQueued{};
            // The producer had to wait for room.
            bool IsWaited{};
        };

        explicit ByteBoundedQueue(std::size_t capacityBytes) : m_CapacityBytes{ capacityBytes } {}
        ~ByteBoundedQueue() = default;

        ByteBoundedQueue(const ByteBoundedQueue&) = delete;
        auto operator=(const ByteBoundedQueue&) -> ByteBoundedQueue& = delete;

        // Opens or closes the queue. Closing wakes a producer that waits for room, and its entry is dropped.
        auto SetOpen(bool value) -> void
        {
            {
                std::lock_guard lock{ m_Mutex };
                m_IsOpen = value;
            }
            m_NotFull.notify_all();
        }

        // param bytes: What the entry keeps alive.
        auto Push(T value, std::size_t bytes) -> PushResult
        {
            PushResult result{};
            {
                std::unique_lock lock{ m_Mutex };
                if (!this->Fits(bytes))
                {
                    result.IsWaited = true;
                    m_NotFull.wait(lock, [this, bytes] { return this->Fits(bytes) || !m_IsOpen; });
                }
                if (!m_IsOpen) return result;
                m_Bytes += bytes;
                m_Entries.push_back(Entry{ std::move(value), bytes });
                result.IsQueued = true;
            }
            m_NotEmpty.notify_one();
            return result;
        }

        // return: The oldest entry, or none after the timeout. Works on a closed queue too, to drain it.
        auto WaitPop(std::chrono::milliseconds timeout) -> std::optional<T>
        {
            std::optional<T> value{};
            {
                std::unique_lock lock{ m_Mutex };
                if (!m_NotEmpty.wait_for(lock, timeout, [this] { return !m_Entries.empty(); })) return std::nullopt;
                value = std::move(m_Entries.front().Value);
                m_Bytes -= m_Entries.front().Bytes;
                m_Entries.pop_front();
            }
            m_NotFull.notify_one();
            return value;
        }

        auto Size() const -> std::size_t
        {
            std::lock_guard lock{ m_Mutex };
            return m_Entries.size();
        }

        auto Bytes() const -> std::size_t
        {
            std::lock_guard lock{ m_Mutex };
            return m_Bytes;
        }

        auto Clear() -> void
        {
            {
                std::lock_guard lock{ m_Mutex };
                m_Entries.clear();
                m_Bytes = 0;
            }
            m_NotFull.notify_all();
        }

    private:
        struct Entry
        {
            T Value{};
            std::size_t Bytes{};
        };

        // note: Called with the mutex held.
        auto Fits(std::size_t bytes) const -> bool
        {
            return m_Entries.empty() || m_Bytes + bytes <= m_CapacityBytes;
        }

        const std::size_t m_CapacityBytes{};
        bool m_IsOpen{};
        std::size_t m_Bytes{};
        std::deque<Entry> m_Entries{};
        mutable std::mutex m_Mutex{};
        std::condition_variable m_NotEmpty{};
        std::condition_variable m_NotFull{};
    };
}