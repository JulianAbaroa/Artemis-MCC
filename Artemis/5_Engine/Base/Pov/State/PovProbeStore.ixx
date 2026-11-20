export module Engine.Base.Pov.State:Probe;

import Engine.Base.Pov.Type;
import std;

namespace
{
    using Engine::Base::Pov::Type::PovProbeDrain;
    using Engine::Base::Pov::Type::PovProbeEvent;
    using Engine::Base::Pov::Type::PovProbeSlot;

    using Engine::Base::Pov::Type::k_PovProbeCreateRingCapacity;
    using Engine::Base::Pov::Type::k_PovProbeExtraWords;
    using Engine::Base::Pov::Type::k_PovProbeMaxThis;
    using Engine::Base::Pov::Type::k_PovProbeRingCapacity;
    using Engine::Base::Pov::Type::k_PovProbeScreenSlots;
    using Engine::Base::Pov::Type::k_PovProbeSlotWords;

    // Words of a slot of a ring: kind, extra count and thread id, the caller, the four arguments, the result and the extra words.
    constexpr std::size_t k_WordMeta{ 0 };
    constexpr std::size_t k_WordCaller{ 1 };
    constexpr std::size_t k_WordArgs{ 2 };
    constexpr std::size_t k_WordResult{ 6 };
    constexpr std::size_t k_WordExtra{ 7 };
    constexpr std::uint32_t k_KindBits{ 8 };
    constexpr std::uint32_t k_CountBits{ 8 };
    constexpr std::uint32_t k_ThreadShift{ k_KindBits + k_CountBits };

    static_assert(k_WordExtra + k_PovProbeExtraWords == k_PovProbeSlotWords, "The slot holds exactly the words of an event.");
}

export namespace Engine::Base::Pov::State
{
    // TEMPORAL VW.9a.x (deleted when VW.9a.x closes): a ring of fixed capacity of noted events. Any thread pushes, one thread drains.
    // note: It never allocates or waits: a writer takes a ticket, fills its slot and publishes the ticket last, and the reader skips a slot whose
    // ticket is not the one it expects (overwritten or not yet published) and counts it as dropped.
    template <std::size_t Capacity>
    class PovProbeRing
    {
    public:
        static_assert((Capacity & (Capacity - 1)) == 0, "The ring index is a mask: the capacity is a power of two.");

        PovProbeRing() = default;
        ~PovProbeRing() = default;

        PovProbeRing(const PovProbeRing&) = delete;
        auto operator=(const PovProbeRing&) -> PovProbeRing& = delete;

        // Notes one event. Any thread, no lock, no allocation.
        auto Push(const PovProbeEvent& event) -> void
        {
            const std::uint32_t extraCount{ (std::min)(event.ExtraCount, static_cast<std::uint32_t>(k_PovProbeExtraWords)) };
            constexpr std::uint64_t mask{ Capacity - 1 };
            const std::uint64_t ticket{ m_Head.fetch_add(1, std::memory_order_relaxed) };
            PovProbeSlot& slot{ m_Slots[ticket & mask] };

            // The ticket goes to zero first, so a reader that meets the slot half written does not take it for the one it waits for.
            slot.Ticket.store(0, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_release);

            slot.Words[k_WordMeta].store(static_cast<std::uint64_t>(event.Kind) | (static_cast<std::uint64_t>(extraCount) << k_KindBits) |
                (static_cast<std::uint64_t>(event.ThreadId) << k_ThreadShift), std::memory_order_relaxed);
            slot.Words[k_WordCaller].store(event.Caller, std::memory_order_relaxed);
            for (std::size_t index{ 0 }; index < event.Args.size(); ++index)
            {
                slot.Words[k_WordArgs + index].store(event.Args[index], std::memory_order_relaxed);
            }

            slot.Words[k_WordResult].store(event.Result, std::memory_order_relaxed);
            for (std::uint32_t index{ 0 }; index < extraCount; ++index)
            {
                slot.Words[k_WordExtra + index].store(event.Extra[index], std::memory_order_relaxed);
            }

            slot.Ticket.store(ticket + 1, std::memory_order_release);
        }

        // Copies the events not read yet, oldest first, into out.
        // return: How many were copied and how many were lost (overwritten before the read or torn by a writer).
        // note: One reading thread.
        auto Drain(std::span<PovProbeEvent> out) -> PovProbeDrain
        {
            constexpr std::uint64_t mask{ Capacity - 1 };
            PovProbeDrain result{};
            const std::uint64_t head{ m_Head.load(std::memory_order_acquire) };

            // The writers lapped the reader: the oldest events are gone.
            if (head - m_Tail > Capacity)
            {
                result.Dropped += static_cast<std::uint32_t>(head - m_Tail - Capacity);
                m_Tail = head - Capacity;
            }

            while (m_Tail < head && result.Count < out.size())
            {
                const PovProbeSlot& slot{ m_Slots[m_Tail & mask] };
                const std::uint64_t expected{ m_Tail + 1 };
                const std::uint64_t ticket{ slot.Ticket.load(std::memory_order_acquire) };

                // A smaller ticket is a writer that took its number and has not published yet: read it in the next call.
                if (ticket < expected) break;

                PovProbeEvent event{};
                bool isRead{ ticket == expected };

                if (isRead)
                {
                    const std::uint64_t meta{ slot.Words[k_WordMeta].load(std::memory_order_relaxed) };
                    event.Kind = static_cast<Engine::Base::Pov::Type::PovProbeKind>(meta & ((1ull << k_KindBits) - 1));
                    event.ExtraCount = static_cast<std::uint32_t>((std::min)((meta >> k_KindBits) & ((1ull << k_CountBits) - 1), k_PovProbeExtraWords));
                    event.ThreadId = static_cast<std::uint32_t>(meta >> k_ThreadShift);
                    event.Caller = slot.Words[k_WordCaller].load(std::memory_order_relaxed);
                    for (std::size_t index{ 0 }; index < event.Args.size(); ++index)
                    {
                        event.Args[index] = slot.Words[k_WordArgs + index].load(std::memory_order_relaxed);
                    }

                    event.Result = slot.Words[k_WordResult].load(std::memory_order_relaxed);
                    for (std::uint32_t index{ 0 }; index < event.ExtraCount; ++index)
                    {
                        event.Extra[index] = slot.Words[k_WordExtra + index].load(std::memory_order_relaxed);
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

        // Forgets the events not read yet. One reading thread.
        auto Discard() -> void
        {
            m_Tail = m_Head.load(std::memory_order_acquire);
        }

    private:
        std::array<PovProbeSlot, Capacity> m_Slots{};
        std::atomic<std::uint64_t> m_Head{ 0 };
        std::uint64_t m_Tail{ 0 };
    };

    // TEMPORAL VW.9a.x (deleted when VW.9a.x closes): what the probe hooks note about who moves the view of user 0, until the game thread
    // writes it to the log. Any thread writes the calls (the hooked functions may run on a thread other than the game thread), the game thread
    // reads. The screens are followed only from the game thread.
    // note: The calls and the screen events go in one ring, the creations of screens in a smaller one of their own, so a burst of creations
    // cannot push the other events out and its losses are counted apart.
    class PovProbeStore
    {
    private:
        using PovProbeCursor = Engine::Base::Pov::Type::PovProbeCursor;
        using PovProbeDrain = Engine::Base::Pov::Type::PovProbeDrain;
        using PovProbeEvent = Engine::Base::Pov::Type::PovProbeEvent;
        using PovProbeGoneScreen = Engine::Base::Pov::Type::PovProbeGoneScreen;
        using PovProbeScreen = Engine::Base::Pov::Type::PovProbeScreen;
        using PovProbeScreenSlot = Engine::Base::Pov::Type::PovProbeScreenSlot;

    public:
        PovProbeStore() = default;
        ~PovProbeStore() = default;

        PovProbeStore(const PovProbeStore&) = delete;
        auto operator=(const PovProbeStore&) -> PovProbeStore& = delete;

        // Whether the hooked functions note their calls. Set by the game thread while the probe is on.
        auto SetActive(bool isActive) -> void;
        auto IsActive() const -> bool;

        // The id of the game thread, so the log can say whether a call came from it and the screens are followed only there.
        auto SetGameThread(std::uint32_t threadId) -> void;
        auto GetGameThread() const -> std::uint32_t;

        // The range of the image of our own DLL, so a stack can skip the frames that are ours.
        auto SetOwnRange(std::uint64_t base, std::uint64_t size) -> void;
        auto IsOwnAddress(std::uint64_t address) const -> bool;

        // Notes one event (a call of a hooked function, a creation of a screen or a change of a screen). Any thread, no lock, no allocation.
        // note: A creation goes in its own ring.
        auto Record(const PovProbeEvent& event) -> void;

        // Notes one call of the cursor candidate: counts it, remembers the first distinct this pointers and the last value read. Any thread.
        auto RecordCursor(std::uint64_t thisPointer, std::uint64_t firstValue) -> void;

        // Copies the calls and screen events not read yet, oldest first, into out.
        // return: How many were copied and how many were lost (overwritten before the read or torn by a writer).
        // note: One reading thread (the game thread).
        auto Drain(std::span<PovProbeEvent> out) -> PovProbeDrain;

        // Same for the ring of the creations of screens.
        auto DrainCreations(std::span<PovProbeEvent> out) -> PovProbeDrain;

        // Forgets the events not read yet, of both rings. One reading thread.
        auto Discard() -> void;

        // The tick the game thread is in, as the probe read it at its last update: the screens use it to say when they were last called.
        auto SetTick(std::uint32_t tick) -> void;

        // Notes one call of the update of a UI screen with its fields at the start of the call (or, with isAfter, at its end).
        // param isAfter: The sample is the one taken when the update returned. Only a screen already in the table is noted then, and a change
        // is a ScreenChangePost event. The calls and the last tick are not counted again.
        // note: Game thread only (the table is not made for two writers). A screen that is not in the table is added and gives a ScreenNew event.
        // When the table is full the screen that was called least recently leaves, with a ScreenEvict event (its Args are the screen that left and
        // the evictions so far) before the ScreenNew. A known screen whose vtable changed is a new screen. A known one whose other fields changed
        // gives a ScreenChange event with the mask of the changed fields, an unchanged one nothing.
        auto NoteScreen(std::uint64_t thisPointer, const PovProbeScreen& screen, std::uint32_t threadId, bool isAfter) -> void;

        // Takes the screen out of the table, so its next update is a new screen (the creator just made it, perhaps at the address of an older one).
        // Game thread only.
        auto ForgetScreen(std::uint64_t thisPointer) -> void;

        // Counts a call of the screen update that came from a thread other than the game thread and was not followed. Any thread.
        auto CountOffThread() -> void;

        // return: The calls counted by CountOffThread since the last take, then starts the count over.
        auto TakeOffThread() -> std::uint32_t;

        // Takes out of the table the screens whose update was not called for k_PovProbeScreenMaxAge ticks before tick.
        // return: How many were copied into out.
        // note: One reading thread (the game thread).
        auto ExpireScreens(std::uint32_t tick, std::span<PovProbeGoneScreen> out) -> std::size_t;

        // Empties the table of screens. One reading thread.
        auto ResetScreens() -> void;

        // return: The cursor candidate calls since the last take, then starts the count over. One reading thread.
        auto TakeCursor() -> PovProbeCursor;

    private:
        PovProbeRing<k_PovProbeRingCapacity> m_Calls{};
        PovProbeRing<k_PovProbeCreateRingCapacity> m_Creations{};

        std::array<PovProbeScreenSlot, k_PovProbeScreenSlots> m_Screens{};
        std::uint64_t m_ScreenBirths{ 0 };
        std::uint64_t m_ScreenEvictions{ 0 };
        std::atomic<std::uint32_t> m_Tick{ 0 };
        std::atomic<std::uint32_t> m_OffThread{ 0 };

        std::atomic<bool> m_IsActive{ false };
        std::atomic<std::uint32_t> m_GameThread{ 0 };
        std::atomic<std::uint64_t> m_OwnBase{ 0 };
        std::atomic<std::uint64_t> m_OwnSize{ 0 };

        std::atomic<std::uint32_t> m_CursorCalls{ 0 };
        std::atomic<std::uint32_t> m_CursorThisCount{ 0 };
        std::array<std::atomic<std::uint64_t>, k_PovProbeMaxThis> m_CursorThis{};
        std::atomic<std::uint64_t> m_CursorFirstValue{ 0 };
    };
}