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

    // How a keyed snapshot relates to the key it was picked for.
    enum class PickKind : std::uint8_t
    {
        // Nothing keyed to pick from.
        None,
        // The key is the target.
        Exact,
        // The nearest key before the target.
        Older,
        // No key at or before the target: the newest one.
        Newer,
    };

    struct PickResult
    {
        PickKind Kind{};
        // Index in the keys given to PickByKey.
        std::size_t Index{};
    };

    // return: The entry whose key is the target, else the one with the greatest key below it, else the one with the greatest key.
    // note: Pure. Entries without a key are ignored. Ties go to the last one.
    inline auto PickByKey(std::span<const std::optional<std::uint32_t>> keys, std::uint32_t target) -> PickResult
    {
        std::optional<std::size_t> older{};
        std::optional<std::size_t> newest{};

        for (std::size_t index{ 0 }; index < keys.size(); ++index)
        {
            if (!keys[index]) continue;

            if (*keys[index] <= target && (!older || *keys[index] >= *keys[*older])) older = index;
            if (!newest || *keys[index] >= *keys[*newest]) newest = index;
        }

        if (older) return PickResult{ .Kind = *keys[*older] == target ? PickKind::Exact : PickKind::Older, .Index = *older };
        if (newest) return PickResult{ .Kind = PickKind::Newer, .Index = *newest };
        return PickResult{};
    }

    // return: True if the entry with the greatest key has a sibling whose key is one less, so the entries arrive one key at a time.
    // note: Pure. Entries without a key are ignored.
    inline auto IsConsecutiveByKey(std::span<const std::optional<std::uint32_t>> keys) -> bool
    {
        std::optional<std::uint32_t> newest{};
        for (const auto& key : keys)
        {
            if (key && (!newest || *key > *newest)) newest = key;
        }

        if (!newest || *newest == 0) return false;

        return std::ranges::any_of(keys, [&](const std::optional<std::uint32_t>& key) { return key && *key == *newest - 1; });
    }

    // The last Capacity values published, each with a key (a tick), handed out as immutable snapshots.
    // param KeyOf: Default-constructible functor, KeyOf{}(const T&) -> std::optional<std::uint32_t>.
    // note: One mutex held only to move pointers (a dropped value is released after the unlock), never while waiting. WaitForKey waits on a condition variable that Publish and Cleanup
    // notify, always with a timeout, so a writer never waits for a reader and a reader never waits without a limit.
    template <typename T, std::size_t Capacity, typename KeyOf>
    class SnapshotHistory
    {
    public:
        struct Pick
        {
            std::shared_ptr<const T> Snapshot{};
            PickKind Kind{};
        };

        SnapshotHistory() = default;
        ~SnapshotHistory() = default;

        SnapshotHistory(const SnapshotHistory&) = delete;
        auto operator=(const SnapshotHistory&) -> SnapshotHistory& = delete;

        // Adds the value as the newest and drops the oldest beyond Capacity.
        auto Publish(T value) -> void
        {
            auto snapshot = std::make_shared<const T>(std::move(value));
            const std::optional<std::uint32_t> key{ KeyOf{}(*snapshot) };

            // The dropped entry may hold the last reference to a large value: it dies after the unlock, never while a reader waits on the mutex.
            Entry dropped{};

            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                m_Entries.push_back(Entry{ .Snapshot = std::move(snapshot), .Key = key });
                if (m_Entries.size() > Capacity)
                {
                    dropped = std::move(m_Entries.front());
                    m_Entries.pop_front();
                }
            }

            m_Changed.notify_all();
        }

        // return: The last published, or null if nothing was published or it was cleaned up.
        auto Acquire() const -> std::shared_ptr<const T>
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            return m_Entries.empty() ? nullptr : m_Entries.back().Snapshot;
        }

        // return: The snapshot PickByKey chooses for the target. Without any key, the last published with Kind None.
        auto PickFor(std::uint32_t target) const -> Pick
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_Entries.empty()) return Pick{};

            const auto keys{ this->KeysLocked() };
            const PickResult result{ PickByKey(keys, target) };
            if (result.Kind == PickKind::None) return Pick{ .Snapshot = m_Entries.back().Snapshot, .Kind = PickKind::None };

            return Pick{ .Snapshot = m_Entries[result.Index].Snapshot, .Kind = result.Kind };
        }

        // return: The snapshot whose key is exactly this one, or null.
        auto FindKey(std::uint32_t key) const -> std::shared_ptr<const T>
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            for (const Entry& entry : m_Entries)
            {
                if (entry.Key && *entry.Key == key) return entry.Snapshot;
            }
            return nullptr;
        }

        // return: Whether the history arrives one key at a time (see IsConsecutiveByKey).
        auto IsConsecutive() const -> bool
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            const auto keys{ this->KeysLocked() };
            return IsConsecutiveByKey(keys);
        }

        // Waits until a snapshot with a key of at least the target is published, or the timeout, or Cleanup.
        // return: True if such a snapshot is there.
        auto WaitForKey(std::uint32_t target, std::chrono::microseconds timeout) -> bool
        {
            std::unique_lock<std::mutex> lock(m_Mutex);
            const std::uint64_t generation{ m_Generation };

            return m_Changed.wait_for(lock, timeout, [&] { return m_Generation != generation || this->HasKeyAtLeastLocked(target); })
                && this->HasKeyAtLeastLocked(target);
        }

        // Drops every snapshot and wakes the waiters. Readers that already acquired one keep their copy.
        auto Cleanup() -> void
        {
            // Same as Publish: the values are released after the unlock.
            std::deque<Entry> dropped{};

            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                dropped.swap(m_Entries);
                ++m_Generation;
            }

            m_Changed.notify_all();
        }

    private:
        struct Entry
        {
            std::shared_ptr<const T> Snapshot{};
            std::optional<std::uint32_t> Key{};
        };

        // note: The caller holds the mutex.
        auto KeysLocked() const -> std::array<std::optional<std::uint32_t>, Capacity>
        {
            std::array<std::optional<std::uint32_t>, Capacity> keys{};
            for (std::size_t index{ 0 }; index < m_Entries.size(); ++index) keys[index] = m_Entries[index].Key;
            return keys;
        }

        // note: The caller holds the mutex.
        auto HasKeyAtLeastLocked(std::uint32_t target) const -> bool
        {
            return std::ranges::any_of(m_Entries, [&](const Entry& entry) { return entry.Key && *entry.Key >= target; });
        }

        mutable std::mutex m_Mutex{};
        std::condition_variable m_Changed{};
        std::deque<Entry> m_Entries{};
        std::uint64_t m_Generation{ 0 };
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
    // A FIFO queue between one producer thread and one consumer thread bounded by the number of entries.
    // The producer waits for room up to a timeout and then gives the entry up (it is not queued and the caller counts it as lost), so a
    // consumer that stopped never holds the producer longer than that. The queue starts closed: Push does nothing until SetOpen(true), and
    // closing it wakes a producer that waits for room.
    template <typename T>
    class BoundedQueue
    {
    public:
        struct PushResult
        {
            // The entry was queued (false if the queue was closed or the wait timed out).
            bool IsQueued{};
            // The producer waited the whole timeout and the queue was still full.
            bool IsTimedOut{};
            // The producer had to wait for room.
            bool IsWaited{};
            // Entries in the queue right after the push, the new one included. 0 if it was not queued.
            std::size_t Depth{};
        };

        explicit BoundedQueue(std::size_t capacity) : m_Capacity{ capacity } {}
        ~BoundedQueue() = default;

        BoundedQueue(const BoundedQueue&) = delete;
        auto operator=(const BoundedQueue&) -> BoundedQueue& = delete;

        // Opens or closes the queue. Closing wakes a producer that waits for room, and its entry is not queued. It does not remove entries.
        auto SetOpen(bool value) -> void
        {
            {
                std::lock_guard lock{ m_Mutex };
                m_IsOpen = value;
            }
            m_NotFull.notify_all();
        }

        auto IsOpen() const -> bool
        {
            std::lock_guard lock{ m_Mutex };
            return m_IsOpen;
        }

        // param timeout: Longest wait for room.
        auto Push(T value, std::chrono::milliseconds timeout) -> PushResult
        {
            PushResult result{};
            {
                std::unique_lock lock{ m_Mutex };
                if (m_IsOpen && m_Entries.size() >= m_Capacity)
                {
                    result.IsWaited = true;
                    m_NotFull.wait_for(lock, timeout, [this] { return m_Entries.size() < m_Capacity || !m_IsOpen; });
                    if (m_IsOpen && m_Entries.size() >= m_Capacity) result.IsTimedOut = true;
                }
                if (!m_IsOpen || result.IsTimedOut) return result;
                m_Entries.push_back(std::move(value));
                result.IsQueued = true;
                result.Depth = m_Entries.size();
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
                value = std::move(m_Entries.front());
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

        // Removes every entry.
        // return: How many were removed.
        auto Clear() -> std::size_t
        {
            std::size_t removed{ 0 };
            {
                std::lock_guard lock{ m_Mutex };
                removed = m_Entries.size();
                m_Entries.clear();
            }
            m_NotFull.notify_all();
            return removed;
        }

    private:
        const std::size_t m_Capacity{};
        bool m_IsOpen{};
        std::deque<T> m_Entries{};
        mutable std::mutex m_Mutex{};
        std::condition_variable m_NotEmpty{};
        std::condition_variable m_NotFull{};
    };
}