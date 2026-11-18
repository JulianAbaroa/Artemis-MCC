#include <gtest/gtest.h>

import Common.Log.System;
import Engine.Base.Memory.Hook;
import Engine.Base.Memory.System;
import Engine.Base.Memory.Type;
import Tests.Support.Log;
import std;

// The read of the engine clock (VW.5c): tick, tick fraction and interpolation from a fake thread context, no game needed.

namespace
{
    using Common::Log::System::Log;
    using Engine::Base::Memory::Hook::FrameClockReader;
    using Engine::Base::Memory::System::AOBService;
    using Engine::Base::Memory::System::MemoryReaderService;
    using Engine::Base::Memory::Type::FrameClock::ClockSource;
    using Engine::Base::Memory::Type::FrameClock::FrameClock;
    using Engine::Base::Memory::Type::FrameClock::InterpolationState;
    using Tests::Support::Log::CollectingSink;

    // Offsets of the thread context and the time globals (A21 and the notes of DatumConstant), written out on purpose: the test checks them.
    constexpr std::size_t k_TimeGlobalsPointer{ 0xA0 };
    constexpr std::size_t k_RenderStatePointer{ 0x70 };
    constexpr std::size_t k_TickOffset{ 0x0C };
    constexpr std::size_t k_FractionOffset{ 0x14 };
    constexpr std::size_t k_InterpolationOffByte{ 0x02 };

    // A thread context with time globals and a render state, and the global that holds the interpolation buffer.
    struct FakeEngine
    {
        std::array<std::uint8_t, 0x100> Context{};
        std::array<std::uint8_t, 0x40> TimeGlobals{};
        std::array<std::uint8_t, 0x10> RenderState{};
        std::uintptr_t InterpolationBuffer{ 0 };

        FakeEngine(std::uint32_t tick, float fraction, std::uintptr_t buffer, std::uint8_t isOff)
        {
            std::memcpy(TimeGlobals.data() + k_TickOffset, &tick, sizeof(tick));
            std::memcpy(TimeGlobals.data() + k_FractionOffset, &fraction, sizeof(fraction));
            RenderState[k_InterpolationOffByte] = isOff;
            InterpolationBuffer = buffer;

            const auto timeGlobals{ reinterpret_cast<std::uintptr_t>(TimeGlobals.data()) };
            const auto renderState{ reinterpret_cast<std::uintptr_t>(RenderState.data()) };
            std::memcpy(Context.data() + k_TimeGlobalsPointer, &timeGlobals, sizeof(timeGlobals));
            std::memcpy(Context.data() + k_RenderStatePointer, &renderState, sizeof(renderState));
        }

        auto ContextAddress() const -> std::uintptr_t { return reinterpret_cast<std::uintptr_t>(Context.data()); }
        auto BufferAddress() const -> std::uintptr_t { return reinterpret_cast<std::uintptr_t>(&InterpolationBuffer); }
    };

    class FrameClockTests : public ::testing::Test
    {
    protected:
        CollectingSink m_Sink{};
        Log m_Log{ m_Sink };
        AOBService m_AOBService{ m_Log };
        MemoryReaderService m_Reader{};
        FrameClockReader m_ClockReader{ m_Reader, m_AOBService };
    };
}

TEST_F(FrameClockTests, ReadsTheTickAndTheFractionOfTheContext)
{
    const FakeEngine engine{ 123456, 0.375f, 0x1000, 0 };

    const FrameClock clock{ m_ClockReader.ReadContext(engine.ContextAddress(), engine.BufferAddress(), ClockSource::Published) };

    EXPECT_EQ(clock.Source, ClockSource::Published);
    EXPECT_EQ(clock.Tick, 123456u);
    EXPECT_FLOAT_EQ(clock.Fraction, 0.375f);
}

TEST_F(FrameClockTests, InterpolatesWithABufferAndTheSwitchOn)
{
    const FakeEngine engine{ 10, 0.0f, 0x1000, 0 };

    EXPECT_EQ(m_ClockReader.ReadContext(engine.ContextAddress(), engine.BufferAddress(), ClockSource::ThreadContext).Interpolation,
        InterpolationState::On);
}

TEST_F(FrameClockTests, DoesNotInterpolateWithoutABufferOrWithTheStateTurnedOff)
{
    const FakeEngine noBuffer{ 10, 0.0f, 0, 0 };
    const FakeEngine turnedOff{ 10, 0.0f, 0x1000, 1 };

    EXPECT_EQ(m_ClockReader.ReadContext(noBuffer.ContextAddress(), noBuffer.BufferAddress(), ClockSource::ThreadContext).Interpolation,
        InterpolationState::Off);
    EXPECT_EQ(m_ClockReader.ReadContext(turnedOff.ContextAddress(), turnedOff.BufferAddress(), ClockSource::ThreadContext).Interpolation,
        InterpolationState::Off);
}

TEST_F(FrameClockTests, InterpolationIsUnknownWithoutTheGlobalAddress)
{
    const FakeEngine engine{ 77, 0.5f, 0x1000, 0 };

    const FrameClock clock{ m_ClockReader.ReadContext(engine.ContextAddress(), 0, ClockSource::ThreadContext) };

    EXPECT_EQ(clock.Tick, 77u);
    EXPECT_EQ(clock.Interpolation, InterpolationState::Unknown);
}

TEST_F(FrameClockTests, AnUnreadableContextGivesNoClock)
{
    FakeEngine engine{ 5, 0.0f, 0x1000, 0 };
    const std::uintptr_t none{ 0 };
    std::memcpy(engine.Context.data() + k_TimeGlobalsPointer, &none, sizeof(none));

    EXPECT_EQ(m_ClockReader.ReadContext(engine.ContextAddress(), engine.BufferAddress(), ClockSource::ThreadContext).Source, ClockSource::None);
    EXPECT_EQ(m_ClockReader.ReadContext(0x10, 0, ClockSource::ThreadContext).Source, ClockSource::None);
    // note: The game module is not loaded in the test process, so there is no TLS slot and no published context either.
    EXPECT_EQ(m_ClockReader.Read(0).Source, ClockSource::None);
}