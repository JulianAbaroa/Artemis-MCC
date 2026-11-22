module Engine.Control.Input.State;
import :PossessRequest;

import std;

namespace
{
    constexpr std::uint64_t k_ActiveBit{ 1ull << 63 };
    constexpr std::uint32_t k_ClockMask{ 0x7FFFFFFFu };
    constexpr int k_ClockShift{ 32 };
    constexpr std::uint32_t k_ClockHalf{ 0x40000000u };

    // return: The clock of a time point in milliseconds, in the 31 bits the word keeps.
    auto ToClock(std::chrono::steady_clock::time_point point) -> std::uint32_t
    {
        const auto milliseconds{ std::chrono::duration_cast<std::chrono::milliseconds>(point.time_since_epoch()).count() };
        return static_cast<std::uint32_t>(milliseconds) & k_ClockMask;
    }
}

namespace Engine::Control::Input::State
{
    auto PossessRequestStore::Publish(std::uint32_t playerHandle, std::chrono::steady_clock::time_point now) -> void
    {
        m_Word.store(k_ActiveBit | (static_cast<std::uint64_t>(ToClock(now)) << k_ClockShift) | playerHandle);
    }

    auto PossessRequestStore::Clear() -> void
    {
        m_Word.store(0);
    }

    auto PossessRequestStore::Read(std::chrono::milliseconds maxAge, std::chrono::steady_clock::time_point now) const -> std::optional<std::uint32_t>
    {
        const std::uint64_t word{ m_Word.load() };
        if ((word & k_ActiveBit) == 0) return std::nullopt;

        const auto stamp{ static_cast<std::uint32_t>(word >> k_ClockShift) & k_ClockMask };

        // note: The difference is signed and taken modulo 2^31, so it holds when the clock wraps and a stamp a few ms ahead of the reader (another
        // thread's clock read) is a fresh request, not an old one.
        const std::uint32_t difference{ (ToClock(now) - stamp) & k_ClockMask };
        const std::int64_t age{ difference >= k_ClockHalf ? static_cast<std::int64_t>(difference) - (static_cast<std::int64_t>(k_ClockMask) + 1)
            : static_cast<std::int64_t>(difference) };
        if (age > maxAge.count()) return std::nullopt;

        return static_cast<std::uint32_t>(word);
    }
}