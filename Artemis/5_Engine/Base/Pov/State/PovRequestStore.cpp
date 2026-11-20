module;

#include <cassert>

module Engine.Base.Pov.State;
import :Request;

import Common.Engine.Type;
import Engine.Base.Memory.Type;
import std;

namespace
{
    using Engine::Base::Memory::Type::Store::k_SlotReadAttempts;

    using Common::Engine::Type::k_NoHandle;

    auto NowNanoseconds() -> std::int64_t
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

namespace Engine::Base::Pov::State
{
    auto PovRequestStore::Write(bool isActive, std::uint32_t playerHandle) -> void
    {
#ifndef NDEBUG
        // The first thread that writes is the writer: any other one breaks the single writer rule.
        std::thread::id expected{};
        m_Writer.compare_exchange_strong(expected, std::this_thread::get_id());
        assert(m_Writer.load() == std::this_thread::get_id());
#endif

        const std::uint32_t next{ m_Current.load(std::memory_order_relaxed) ^ 1u };
        PovRequestSlot& slot{ m_Slots[next] };

        const std::uint32_t sequence{ slot.Sequence.load(std::memory_order_relaxed) };

        // An odd sequence tells a reader of this buffer that a write is in progress.
        slot.Sequence.store(sequence + 1, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);

        slot.PlayerHandle.store(playerHandle, std::memory_order_relaxed);
        slot.IsActive.store(isActive, std::memory_order_relaxed);
        slot.StampNanoseconds.store(NowNanoseconds(), std::memory_order_relaxed);
        slot.Sequence.store(sequence + 2, std::memory_order_release);

        m_Current.store(next, std::memory_order_release);
    }

    auto PovRequestStore::Publish(std::uint32_t playerHandle) -> void
    {
        this->Write(true, playerHandle);
    }

    auto PovRequestStore::Clear() -> void
    {
        this->Write(false, k_NoHandle);
    }

    auto PovRequestStore::Read(std::chrono::nanoseconds maxAge) const -> std::optional<PovRequest>
    {
        for (int attempt{ 0 }; attempt < k_SlotReadAttempts; ++attempt)
        {
            const PovRequestSlot& slot{ m_Slots[m_Current.load(std::memory_order_acquire) & 1u] };

            const std::uint32_t before{ slot.Sequence.load(std::memory_order_acquire) };
            if ((before & 1u) != 0) continue;

            const std::uint32_t playerHandle{ slot.PlayerHandle.load(std::memory_order_relaxed) };
            const bool isActive{ slot.IsActive.load(std::memory_order_relaxed) };
            const std::int64_t stamp{ slot.StampNanoseconds.load(std::memory_order_relaxed) };

            std::atomic_thread_fence(std::memory_order_acquire);
            if (slot.Sequence.load(std::memory_order_relaxed) != before) continue;

            if (!isActive) return std::nullopt;
            if (NowNanoseconds() - stamp > maxAge.count()) return std::nullopt;

            return PovRequest{ .PlayerHandle = playerHandle };
        }

        return std::nullopt;
    }
}