export module Common.State.System;

import std;

export namespace Common::State::System
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
}