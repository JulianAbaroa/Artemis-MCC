export module DevTools.Viewer.Latency.State;

import DevTools.Viewer.Latency.Type;
import std;

export namespace DevTools::Viewer::Latency::State
{
    // The last frames of the overlay with what they saw of the clock and the scene, and what the next frame needs from the previous ones.
    // note: Thread safe: the render thread pushes and the panel reads, both under one short mutex. Nothing here waits for the game.
    class LatencyStore
    {
    private:
        using LatencyMemory = DevTools::Viewer::Latency::Type::LatencyMemory;
        using LatencySample = DevTools::Viewer::Latency::Type::LatencySample;

    public:
        LatencyStore() = default;
        ~LatencyStore() = default;

        LatencyStore(const LatencyStore&) = delete;
        auto operator=(const LatencyStore&) -> LatencyStore& = delete;

        // Adds a frame, drops the oldest beyond k_WindowFrames and keeps the memory for the next frame.
        auto Push(const LatencySample& sample, const LatencyMemory& memory) -> void;

        auto GetMemory() const -> LatencyMemory;

        // return: The frames of the window, the oldest first.
        auto GetSamples() const -> std::vector<LatencySample>;

    private:
        mutable std::mutex m_Mutex{};
        std::deque<LatencySample> m_Samples{};
        LatencyMemory m_Memory{};
    };
}