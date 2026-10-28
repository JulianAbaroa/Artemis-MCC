export module UI.TickTrace.Type;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace UI::TickTrace::Type
{
    // Version of the trace file layout. Tools/analyze_ticktrace.py reads the same layout.
    inline constexpr std::uint32_t k_TraceVersion{ 1 };

    // Engine clock and event counter of a tick, read inside the read window of the AI thread.
    struct TickClock
    {
        // Ticks since the match started (time globals +0xC).
        std::uint32_t EngineTick{ 0 };
        // Engine speed (time globals +0x10).
        float TimeScale{ 0.0f };
        // Fraction of a tick left in the accumulator (time globals +0x14).
        float TickFraction{ 0.0f };
        // Last sequence of the event ring (D104).
        std::uint32_t EventSequence{ 0 };
        // False when the thread context or the time globals were not available.
        bool IsValid{ false };
    };

    // Start of the trace file.
    struct TraceFileHeader
    {
        std::array<char, 8> Magic{ 'A', 'T', 'T', 'R', 'A', 'C', 'E', '\0' };
        std::uint32_t Version{ k_TraceVersion };
        std::uint32_t TickRecordSize{ 0 };
        std::uint32_t ObjectRecordSize{ 0 };
        std::uint32_t Reserved{ 0 };
        // Map file name without extension, ASCII, zero padded.
        std::array<char, 64> MapName{};
        // Local start time "YYYY-MM-DD HH:MM:SS", zero padded.
        std::array<char, 32> StartTime{};
    };

    static_assert(sizeof(TraceFileHeader) == 120);

    // One tick. ObjectCount object records follow it.
    struct TickRecord
    {
        // Tick generation of Artemis (counts the signaled ticks).
        std::uint64_t Generation{ 0 };
        // Real time since the recording started, in nanoseconds.
        std::uint64_t RealTimeNs{ 0 };
        std::uint32_t EngineTick{ 0 };
        float TimeScale{ 0.0f };
        float TickFraction{ 0.0f };
        std::uint32_t EventSequence{ 0 };
        std::uint32_t ObjectCount{ 0 };
        // Bit 0: the clock fields are valid.
        std::uint32_t Flags{ 0 };
    };

    static_assert(sizeof(TickRecord) == 40);

    // One traced object (bipeds, vehicles, projectiles and weapons), from the object table of the tick. Sorted by handle.
    struct ObjectRecord
    {
        std::uint32_t Handle{ 0 };
        std::uint32_t ParentHandle{ 0 };
        std::array<char, 4> FourCC{};
        float DamageReceived{ 0.0f };
        Vec3 BoundingCenter{};
        Vec3 WorldOrigin{};
        Vec3 Forward{};
        Vec3 Up{};
        Vec3 LinearVelocity{};
        Vec3 AngularVelocity{};
    };

    static_assert(sizeof(ObjectRecord) == 88);
}