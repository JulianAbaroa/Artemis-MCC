module Engine.Base.Pov.State;
import :Probe;

import std;

namespace
{
    using Engine::Base::Pov::Type::PovProbeCursor;
    using Engine::Base::Pov::Type::PovProbeDrain;
    using Engine::Base::Pov::Type::PovProbeEvent;
    using Engine::Base::Pov::Type::PovProbeKind;

    using Engine::Base::Pov::Type::k_PovProbeMaxThis;
    using Engine::Base::Pov::Type::k_PovProbeRingCapacity;

    static_assert((k_PovProbeRingCapacity & (k_PovProbeRingCapacity - 1)) == 0, "The ring index is a mask: the capacity is a power of two.");

    constexpr std::uint64_t k_RingMask{ k_PovProbeRingCapacity - 1 };

    // Words of a slot: kind and thread id, the caller, the four arguments.
    constexpr std::size_t k_WordMeta{ 0 };
    constexpr std::size_t k_WordCaller{ 1 };
    constexpr std::size_t k_WordArgs{ 2 };
    constexpr std::uint32_t k_KindBits{ 8 };
}

namespace Engine::Base::Pov::State
{
    auto PovProbeStore::SetActive(bool isActive) -> void
    {
        m_IsActive.store(isActive, std::memory_order_release);
    }

    auto PovProbeStore::IsActive() const -> bool
    {
        return m_IsActive.load(std::memory_order_acquire);
    }

    auto PovProbeStore::SetGameThread(std::uint32_t threadId) -> void
    {
        m_GameThread.store(threadId, std::memory_order_relaxed);
    }

    auto PovProbeStore::GetGameThread() const -> std::uint32_t
    {
        return m_GameThread.load(std::memory_order_relaxed);
    }

    auto PovProbeStore::Record(PovProbeKind kind, std::uint32_t threadId, std::uint64_t caller, const std::array<std::uint64_t, 4>& args) -> void
    {
        const std::uint64_t ticket{ m_Head.fetch_add(1, std::memory_order_relaxed) };
        PovProbeSlot& slot{ m_Slots[ticket & k_RingMask] };

        // The ticket goes to zero first, so a reader that meets the slot half written does not take it for the one it waits for.
        slot.Ticket.store(0, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);

        slot.Words[k_WordMeta].store(static_cast<std::uint64_t>(kind) | (static_cast<std::uint64_t>(threadId) << k_KindBits), std::memory_order_relaxed);
        slot.Words[k_WordCaller].store(caller, std::memory_order_relaxed);
        for (std::size_t index{ 0 }; index < args.size(); ++index)
        {
            slot.Words[k_WordArgs + index].store(args[index], std::memory_order_relaxed);
        }

        slot.Ticket.store(ticket + 1, std::memory_order_release);
    }

    auto PovProbeStore::RecordCursor(std::uint64_t thisPointer, std::uint64_t firstValue) -> void
    {
        m_CursorCalls.fetch_add(1, std::memory_order_relaxed);
        m_CursorFirstValue.store(firstValue, std::memory_order_relaxed);

        const std::uint32_t known{ (std::min)(m_CursorThisCount.load(std::memory_order_relaxed), static_cast<std::uint32_t>(k_PovProbeMaxThis)) };
        for (std::uint32_t index{ 0 }; index < known; ++index)
        {
            if (m_CursorThis[index].load(std::memory_order_relaxed) == thisPointer) return;
        }

        if (known >= k_PovProbeMaxThis) return;

        // Two threads can take the same index: the last writer wins, a lost pointer is not worth a lock.
        const std::uint32_t index{ m_CursorThisCount.fetch_add(1, std::memory_order_relaxed) };
        if (index < k_PovProbeMaxThis) m_CursorThis[index].store(thisPointer, std::memory_order_relaxed);
    }

    auto PovProbeStore::Drain(std::span<PovProbeEvent> out) -> PovProbeDrain
    {
        PovProbeDrain result{};
        const std::uint64_t head{ m_Head.load(std::memory_order_acquire) };

        // The writers lapped the reader: the oldest events are gone.
        if (head - m_Tail > k_PovProbeRingCapacity)
        {
            result.Dropped += static_cast<std::uint32_t>(head - m_Tail - k_PovProbeRingCapacity);
            m_Tail = head - k_PovProbeRingCapacity;
        }

        while (m_Tail < head && result.Count < out.size())
        {
            const PovProbeSlot& slot{ m_Slots[m_Tail & k_RingMask] };
            const std::uint64_t expected{ m_Tail + 1 };
            const std::uint64_t ticket{ slot.Ticket.load(std::memory_order_acquire) };

            // A smaller ticket is a writer that took its number and has not published yet: read it in the next call.
            if (ticket < expected) break;

            PovProbeEvent event{};
            bool isRead{ ticket == expected };

            if (isRead)
            {
                const std::uint64_t meta{ slot.Words[k_WordMeta].load(std::memory_order_relaxed) };
                event.Kind = static_cast<PovProbeKind>(meta & ((1ull << k_KindBits) - 1));
                event.ThreadId = static_cast<std::uint32_t>(meta >> k_KindBits);
                event.Caller = slot.Words[k_WordCaller].load(std::memory_order_relaxed);
                for (std::size_t index{ 0 }; index < event.Args.size(); ++index)
                {
                    event.Args[index] = slot.Words[k_WordArgs + index].load(std::memory_order_relaxed);
                }

                std::atomic_thread_fence(std::memory_order_acquire);
                isRead = slot.Ticket.load(std::memory_order_relaxed) == expected;
            }

            if (isRead) out[result.Count++] = event;
            else ++result.Dropped;

            ++m_Tail;
        }

        return result;
    }

    auto PovProbeStore::Discard() -> void
    {
        m_Tail = m_Head.load(std::memory_order_acquire);
    }

    auto PovProbeStore::TakeCursor() -> PovProbeCursor
    {
        PovProbeCursor cursor{};
        cursor.Calls = m_CursorCalls.exchange(0, std::memory_order_relaxed);
        cursor.ThisCount = (std::min)(m_CursorThisCount.exchange(0, std::memory_order_relaxed), static_cast<std::uint32_t>(k_PovProbeMaxThis));

        for (std::uint32_t index{ 0 }; index < cursor.ThisCount; ++index)
        {
            cursor.This[index] = m_CursorThis[index].load(std::memory_order_relaxed);
        }

        cursor.FirstValue = m_CursorFirstValue.load(std::memory_order_relaxed);
        return cursor;
    }
}