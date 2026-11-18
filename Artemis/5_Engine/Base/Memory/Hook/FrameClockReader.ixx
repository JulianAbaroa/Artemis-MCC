export module Engine.Base.Memory.Hook:FrameClock;

import Engine.Base.Memory.Type;
import Engine.Base.Memory.System;
import std;

export namespace Engine::Base::Memory::Hook
{
    // Reads the time of the engine (tick, tick fraction, interpolation) without calling any engine function (VW.5c).
    // note: One reading thread (the render thread of the Viewer): it keeps the TLS slot it resolved for the module that is loaded. Memory reads
    // never fault. It takes no lock and never waits for the game thread.
    class FrameClockReader
    {
    private:
        using ClockSource = Engine::Base::Memory::Type::FrameClock::ClockSource;
        using FrameClock = Engine::Base::Memory::Type::FrameClock::FrameClock;
        using InterpolationState = Engine::Base::Memory::Type::FrameClock::InterpolationState;

        using AOBService = Engine::Base::Memory::System::AOBService;
        using MemoryReaderService = Engine::Base::Memory::System::MemoryReaderService;

    public:
        FrameClockReader(MemoryReaderService& reader, AOBService& aobService) : m_Reader(reader), m_AOBService(aobService) {}
        ~FrameClockReader() = default;

        FrameClockReader(const FrameClockReader&) = delete;
        auto operator=(const FrameClockReader&) -> FrameClockReader& = delete;

        // Reads the clock through the thread context of the calling thread, or through the published one when this thread has none.
        // param publishedContext: The thread context the game thread published, 0 if none.
        // return: The clock, with Source None when no context can be read.
        auto Read(std::uintptr_t publishedContext) -> FrameClock;

        // Reads the clock of one thread context.
        // param context: Thread context of the engine.
        // param interpolationBuffer: Address of the global that holds the interpolation buffer pointer, 0 to leave the interpolation unknown.
        // return: The clock with Source None when the time globals cannot be read.
        auto ReadContext(std::uintptr_t context, std::uintptr_t interpolationBuffer, ClockSource source) -> FrameClock;

    private:
        auto ReadInterpolation(std::uintptr_t context, std::uintptr_t interpolationBuffer) -> InterpolationState;

        MemoryReaderService& m_Reader;
        AOBService& m_AOBService;

        std::uintptr_t m_Module{ 0 };
        const std::uint32_t* m_TlsSlot{ nullptr };
    };
}