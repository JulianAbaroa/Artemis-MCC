export module Engine.Base.Memory.Type:FrameClock;

import std;

export namespace Engine::Base::Memory::Type::FrameClock
{
    // Pointer of the thread context to the render state, and the byte of it whose value other than 0 turns the interpolation of the frame off
    // (VW.5c: RE of the decompiled game, not verified live).
    inline constexpr std::uintptr_t k_ThreadContextRenderStateOffset{ 0x70 };
    inline constexpr std::uintptr_t k_RenderStateInterpolationOffOffset{ 0x02 };

    // Where the clock was read from.
    enum class ClockSource : std::uint8_t
    {
        // Nothing could be read.
        None,
        // The thread context of the calling thread (the engine TLS slot holds one).
        ThreadContext,
        // The thread context the game thread published, read from another thread.
        Published,
    };

    // Whether the game draws the frame interpolated between two ticks.
    enum class InterpolationState : std::uint8_t
    {
        Unknown,
        Off,
        On,
    };

    // The time of the engine at the moment of the read.
    // note: Fraction is the raw tick fraction accumulated since the last tick (not clamped). The reads are not atomic with the game thread: the
    // tick and the fraction can come from two different instants.
    struct FrameClock
    {
        ClockSource Source{ ClockSource::None };
        std::uint32_t Tick{};
        float Fraction{};
        InterpolationState Interpolation{ InterpolationState::Unknown };
    };
}