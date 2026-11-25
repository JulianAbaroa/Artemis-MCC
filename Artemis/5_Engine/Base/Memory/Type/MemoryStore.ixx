export module Engine.Base.Memory.Type:Store;

import std;

export namespace Engine::Base::Memory::Type::Store
{
    // Times a reader of a double buffered store (CameraOverrideStore, PovRequestStore) retries when the writer laps it, before it gives up.
    inline constexpr int k_SlotReadAttempts{ 4 };

    // A gap between two reads of a store longer than this is a stall of its readers (the game thread stood still, as while Lockstep waits
    // for the PPO update of the client), not a frame (D379).
    inline constexpr std::int64_t k_ReaderStallNanoseconds{ 50'000'000 };

    // The time the readers of a double buffered store stood still since its last write (D379). The age of a value is measured without it:
    // the writer (the Viewer at Present) cannot publish while the game thread is stopped, so after a stall the last value is still the
    // current one. Only a writer that stops while the readers keep running lets its value age out.
    // note: Lock free and without allocations. Reset by the writer, advanced by the readers (any thread).
    class ReaderStallClock
    {
    public:
        // Forgets the stalls, because a new value was just written.
        auto Reset() -> void
        {
            m_StalledNanoseconds.store(0, std::memory_order_relaxed);
        }

        // Notes a read made at nowNanoseconds (steady clock).
        // return: The stalled time since the last Reset, this read included.
        auto Read(std::int64_t nowNanoseconds) -> std::int64_t
        {
            const std::int64_t previous{ m_LastReadNanoseconds.exchange(nowNanoseconds, std::memory_order_relaxed) };
            const std::int64_t gap{ previous == 0 ? 0 : nowNanoseconds - previous };
            if (gap <= k_ReaderStallNanoseconds) return m_StalledNanoseconds.load(std::memory_order_relaxed);
            return m_StalledNanoseconds.fetch_add(gap, std::memory_order_relaxed) + gap;
        }

    private:
        std::atomic<std::int64_t> m_LastReadNanoseconds{ 0 };
        std::atomic<std::int64_t> m_StalledNanoseconds{ 0 };
    };
}