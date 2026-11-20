module;

#include <cassert>

module Engine.Base.Memory.State;
import :CameraOverride;

import std;

namespace
{
    using Engine::Base::Memory::Type::Store::k_SlotReadAttempts;

    auto NowNanoseconds() -> std::int64_t
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }
}

namespace Engine::Base::Memory::State
{
    auto CameraOverrideStore::Write(bool isActive, const CameraOverride& pose) -> void
    {
#ifndef NDEBUG
        // The first thread that writes is the writer: any other one breaks the single writer rule.
        std::thread::id expected{};
        m_Writer.compare_exchange_strong(expected, std::this_thread::get_id());
        assert(m_Writer.load() == std::this_thread::get_id());
#endif

        const std::uint32_t next{ m_Current.load(std::memory_order_relaxed) ^ 1u };
        CameraOverrideSlot& slot{ m_Slots[next] };

        const std::uint32_t sequence{ slot.Sequence.load(std::memory_order_relaxed) };

        // An odd sequence tells a reader of this buffer that a write is in progress.
        slot.Sequence.store(sequence + 1, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);

        const std::array<float, 9> values{
            pose.Eye.X, pose.Eye.Y, pose.Eye.Z,
            pose.Forward.X, pose.Forward.Y, pose.Forward.Z,
            pose.Up.X, pose.Up.Y, pose.Up.Z };

        for (std::size_t i{ 0 }; i < values.size(); ++i) slot.Values[i].store(values[i], std::memory_order_relaxed);

        slot.IsActive.store(isActive, std::memory_order_relaxed);
        slot.StampNanoseconds.store(NowNanoseconds(), std::memory_order_relaxed);
        slot.Sequence.store(sequence + 2, std::memory_order_release);

        m_Current.store(next, std::memory_order_release);
    }

    auto CameraOverrideStore::Publish(const CameraOverride& pose) -> void
    {
        this->Write(true, pose);
    }

    auto CameraOverrideStore::Clear() -> void
    {
        this->Write(false, CameraOverride{});
    }

    auto CameraOverrideStore::Read(std::chrono::nanoseconds maxAge) const -> std::optional<CameraOverride>
    {
        for (int attempt{ 0 }; attempt < k_SlotReadAttempts; ++attempt)
        {
            const CameraOverrideSlot& slot{ m_Slots[m_Current.load(std::memory_order_acquire) & 1u] };

            const std::uint32_t before{ slot.Sequence.load(std::memory_order_acquire) };
            if ((before & 1u) != 0) continue;

            std::array<float, 9> values{};
            for (std::size_t i{ 0 }; i < values.size(); ++i) values[i] = slot.Values[i].load(std::memory_order_relaxed);
            const bool isActive{ slot.IsActive.load(std::memory_order_relaxed) };
            const std::int64_t stamp{ slot.StampNanoseconds.load(std::memory_order_relaxed) };

            std::atomic_thread_fence(std::memory_order_acquire);
            if (slot.Sequence.load(std::memory_order_relaxed) != before) continue;

            if (!isActive) return std::nullopt;
            if (NowNanoseconds() - stamp > maxAge.count()) return std::nullopt;

            CameraOverride pose{};
            pose.Eye = { values[0], values[1], values[2] };
            pose.Forward = { values[3], values[4], values[5] };
            pose.Up = { values[6], values[7], values[8] };
            return pose;
        }

        return std::nullopt;
    }

    auto CameraOverrideStore::IsActive() const -> bool
    {
        return m_Slots[m_Current.load(std::memory_order_acquire) & 1u].IsActive.load(std::memory_order_acquire);
    }

    auto CameraOverrideStore::CountFault() -> void
    {
        m_FaultCount.fetch_add(1, std::memory_order_relaxed);
    }

    auto CameraOverrideStore::GetFaultCount() const -> std::uint32_t
    {
        return m_FaultCount.load(std::memory_order_relaxed);
    }
}