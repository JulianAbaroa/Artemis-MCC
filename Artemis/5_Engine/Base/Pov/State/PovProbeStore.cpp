module Engine.Base.Pov.State;
import :Probe;

import std;

namespace
{
    using Engine::Base::Pov::Type::PovProbeCursor;
    using Engine::Base::Pov::Type::PovProbeDrain;
    using Engine::Base::Pov::Type::PovProbeEvent;
    using Engine::Base::Pov::Type::PovProbeGoneScreen;
    using Engine::Base::Pov::Type::PovProbeKind;
    using Engine::Base::Pov::Type::PovProbeScreen;
    using Engine::Base::Pov::Type::PovProbeScreenField;
    using Engine::Base::Pov::Type::PovProbeScreenSlot;

    using Engine::Base::Pov::Type::k_PovProbeMaxThis;
    using Engine::Base::Pov::Type::k_PovProbeScreenFieldCount;
    using Engine::Base::Pov::Type::k_PovProbeScreenMaxAge;

    // A screen that is not in the table yet: every field counts as changed.
    constexpr std::uint64_t k_AllFields{ (1ull << k_PovProbeScreenFieldCount) - 1 };

    constexpr std::size_t k_VtableField{ static_cast<std::size_t>(PovProbeScreenField::Vtable) };

    // return: Whether the kind is a creation of a screen.
    auto IsCreation(PovProbeKind kind) -> bool
    {
        return kind == PovProbeKind::ScreenCreate || kind == PovProbeKind::ScreenCreateAlt || kind == PovProbeKind::ScreenCreateEx;
    }

    // Puts a screen in a slot as a new one.
    auto FillSlot(PovProbeScreenSlot& slot, std::uint64_t thisPointer, const PovProbeScreen& screen, std::uint64_t birth, std::uint32_t tick) -> void
    {
        slot.This.store(0, std::memory_order_relaxed);
        for (std::size_t index{ 0 }; index < k_PovProbeScreenFieldCount; ++index)
        {
            slot.Values[index].store(screen.Values[index], std::memory_order_relaxed);
        }

        slot.Birth.store(birth, std::memory_order_relaxed);
        slot.FirstTick.store(tick, std::memory_order_relaxed);
        slot.LastTick.store(tick, std::memory_order_relaxed);
        slot.Calls.store(1, std::memory_order_relaxed);
        slot.This.store(thisPointer, std::memory_order_release);
    }
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

    auto PovProbeStore::SetOwnRange(std::uint64_t base, std::uint64_t size) -> void
    {
        m_OwnBase.store(base, std::memory_order_relaxed);
        m_OwnSize.store(size, std::memory_order_relaxed);
    }

    auto PovProbeStore::IsOwnAddress(std::uint64_t address) const -> bool
    {
        const std::uint64_t base{ m_OwnBase.load(std::memory_order_relaxed) };
        return base != 0 && address >= base && address < base + m_OwnSize.load(std::memory_order_relaxed);
    }

    auto PovProbeStore::Record(const PovProbeEvent& event) -> void
    {
        if (IsCreation(event.Kind)) m_Creations.Push(event);
        else m_Calls.Push(event);
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
        return m_Calls.Drain(out);
    }

    auto PovProbeStore::DrainCreations(std::span<PovProbeEvent> out) -> PovProbeDrain
    {
        return m_Creations.Drain(out);
    }

    auto PovProbeStore::Discard() -> void
    {
        m_Calls.Discard();
        m_Creations.Discard();
    }

    auto PovProbeStore::SetTick(std::uint32_t tick) -> void
    {
        m_Tick.store(tick, std::memory_order_relaxed);
    }

    auto PovProbeStore::NoteScreen(std::uint64_t thisPointer, const PovProbeScreen& screen, std::uint32_t threadId, bool isAfter) -> void
    {
        const std::uint32_t tick{ m_Tick.load(std::memory_order_relaxed) };

        PovProbeScreenSlot* found{ nullptr };
        PovProbeScreenSlot* vacant{ nullptr };
        PovProbeScreenSlot* leastRecent{ nullptr };
        for (PovProbeScreenSlot& slot : m_Screens)
        {
            const std::uint64_t owner{ slot.This.load(std::memory_order_relaxed) };
            if (owner == thisPointer)
            {
                found = &slot;
                break;
            }

            if (owner == 0)
            {
                if (vacant == nullptr) vacant = &slot;
            }
            else if (leastRecent == nullptr || slot.LastTick.load(std::memory_order_relaxed) < leastRecent->LastTick.load(std::memory_order_relaxed) ||
                (slot.LastTick.load(std::memory_order_relaxed) == leastRecent->LastTick.load(std::memory_order_relaxed) &&
                    slot.Birth.load(std::memory_order_relaxed) < leastRecent->Birth.load(std::memory_order_relaxed)))
            {
                leastRecent = &slot;
            }
        }

        // At the end of an update only a screen that was noted at its start matters.
        if (isAfter && found == nullptr) return;

        PovProbeEvent event{};
        event.ThreadId = threadId;
        event.ExtraCount = static_cast<std::uint32_t>(k_PovProbeScreenFieldCount);
        std::copy(screen.Values.begin(), screen.Values.end(), event.Extra.begin());
        event.Args[0] = thisPointer;

        // A new screen, or a known address with another vtable (the old screen was destroyed and another took its place).
        const bool isReborn{ found != nullptr && !isAfter && found->Values[k_VtableField].load(std::memory_order_relaxed) != screen.Values[k_VtableField] };
        if (found == nullptr || isReborn)
        {
            PovProbeScreenSlot* slot{ found };
            if (slot == nullptr && vacant != nullptr) slot = vacant;

            if (slot == nullptr)
            {
                // The table is full: the screen called least recently leaves.
                slot = leastRecent;
                PovProbeEvent evict{};
                evict.Kind = PovProbeKind::ScreenEvict;
                evict.ThreadId = threadId;
                evict.Args[0] = slot->This.load(std::memory_order_relaxed);
                evict.Args[1] = ++m_ScreenEvictions;
                this->Record(evict);
            }

            FillSlot(*slot, thisPointer, screen, ++m_ScreenBirths, tick);

            event.Kind = PovProbeKind::ScreenNew;
            event.Args[1] = k_AllFields;
            this->Record(event);
            return;
        }

        if (!isAfter)
        {
            found->Calls.fetch_add(1, std::memory_order_relaxed);
            found->LastTick.store(tick, std::memory_order_relaxed);
        }

        std::uint64_t changed{ 0 };
        for (std::size_t index{ 0 }; index < k_PovProbeScreenFieldCount; ++index)
        {
            if (found->Values[index].load(std::memory_order_relaxed) == screen.Values[index]) continue;

            found->Values[index].store(screen.Values[index], std::memory_order_relaxed);
            changed |= 1ull << index;
        }

        if (changed == 0) return;

        event.Kind = isAfter ? PovProbeKind::ScreenChangePost : PovProbeKind::ScreenChange;
        event.Args[1] = changed;
        this->Record(event);
    }

    auto PovProbeStore::ForgetScreen(std::uint64_t thisPointer) -> void
    {
        for (PovProbeScreenSlot& slot : m_Screens)
        {
            if (slot.This.load(std::memory_order_relaxed) == thisPointer) slot.This.store(0, std::memory_order_release);
        }
    }

    auto PovProbeStore::CountOffThread() -> void
    {
        m_OffThread.fetch_add(1, std::memory_order_relaxed);
    }

    auto PovProbeStore::TakeOffThread() -> std::uint32_t
    {
        return m_OffThread.exchange(0, std::memory_order_relaxed);
    }

    auto PovProbeStore::ExpireScreens(std::uint32_t tick, std::span<PovProbeGoneScreen> out) -> std::size_t
    {
        std::size_t count{ 0 };
        for (PovProbeScreenSlot& slot : m_Screens)
        {
            if (count >= out.size()) break;

            const std::uint64_t owner{ slot.This.load(std::memory_order_acquire) };
            const std::uint32_t lastTick{ slot.LastTick.load(std::memory_order_relaxed) };
            if (owner == 0 || tick < lastTick || tick - lastTick < k_PovProbeScreenMaxAge) continue;

            out[count++] = PovProbeGoneScreen{ .This = owner, .FirstTick = slot.FirstTick.load(std::memory_order_relaxed), .LastTick = lastTick,
                .Calls = slot.Calls.load(std::memory_order_relaxed) };
            slot.This.store(0, std::memory_order_release);
        }

        return count;
    }

    auto PovProbeStore::ResetScreens() -> void
    {
        for (PovProbeScreenSlot& slot : m_Screens) slot.This.store(0, std::memory_order_release);
        m_OffThread.store(0, std::memory_order_relaxed);
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