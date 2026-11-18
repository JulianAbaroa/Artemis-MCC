export module DevTools.Viewer.Latency.System;

import Common.Math.Type;
import Engine.Base.Memory.Type;
import Engine.Base.Memory.Hook;
import Engine.Capture.State;
import DevTools.Viewer.Latency.Type;
import DevTools.Viewer.Latency.State;
import std;

export namespace DevTools::Viewer::Latency::System
{
    // Measures how far the scene the overlay draws is from the frame of the game (VW.5c): the clock of the engine at the moment of drawing against
    // the tick of the scene, and whether the game camera moves inside a tick (the game interpolates). It changes nothing that is drawn.
    // note: Record and ReadClock run on the thread that draws the overlay (the game render thread under the HUD, or Present). The clock is read
    // through the TLS of that thread when it has the engine context, otherwise through the context the game thread published.
    class LatencyService
    {
    private:
        using Vec3 = Common::Math::Type::Vec3;

        using FrameClock = Engine::Base::Memory::Type::FrameClock::FrameClock;
        using FrameClockReader = Engine::Base::Memory::Hook::FrameClockReader;
        using CaptureStore = Engine::Capture::State::CaptureStore;

        using LatencyInput = DevTools::Viewer::Latency::Type::LatencyInput;
        using LatencyMemory = DevTools::Viewer::Latency::Type::LatencyMemory;
        using LatencyReport = DevTools::Viewer::Latency::Type::LatencyReport;
        using LatencySample = DevTools::Viewer::Latency::Type::LatencySample;

        using LatencyStore = DevTools::Viewer::Latency::State::LatencyStore;

    public:
        LatencyService(LatencyStore& store, FrameClockReader& clockReader, CaptureStore& captureStore) :
            m_Store(store), m_ClockReader(clockReader), m_CaptureStore(captureStore) {}
        ~LatencyService() = default;

        LatencyService(const LatencyService&) = delete;
        auto operator=(const LatencyService&) -> LatencyService& = delete;

        // return: The clock of the engine now, through the TLS of the calling thread when it has the engine context, else the published one.
        // note: Memory reads only, no lock, never waits for the game thread. MapService reads it once per frame, before it picks the scene, and
        // passes the same value to Record.
        auto ReadClock() -> FrameClock;

        // Records the frame.
        // param input: What the overlay saw, with the clock ReadClock gave before the scene was picked.
        auto Record(const LatencyInput& input) -> void;

        // return: The window summarized.
        auto GetReport() const -> LatencyReport;

        // return: The sample of one frame, and the memory for the next one updated in place.
        // note: Pure. The lag is the tick of the game minus the tick of the scene. The probe needs the eyes of two consecutive scenes, and compares the
        // game camera with the eye of the scene, the eye before it and the eye interpolated with the tick fraction clamped to 0..1.
        static auto Observe(LatencyMemory& memory, const LatencyInput& input) -> LatencySample;

        // return: The histogram, the camera counters and the percentiles of the frames of the window.
        // note: Pure. A percentile is the nearest rank.
        static auto Summarize(std::span<const LatencySample> samples) -> LatencyReport;

    private:
        LatencyStore& m_Store;
        FrameClockReader& m_ClockReader;
        CaptureStore& m_CaptureStore;
    };
}