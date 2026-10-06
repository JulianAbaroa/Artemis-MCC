module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Profiler.System;

import Service.Logs.System;
import Viewer.Profiler.Type;
import std;

namespace
{
    using Viewer::Profiler::Type::k_CpuCount;
    using Viewer::Profiler::Type::k_GpuCount;
    using Viewer::Profiler::Type::k_CounterCount;
}

export namespace Viewer::Profiler::System
{
    // Measures the CPU and GPU time of the map frame and logs the averages once per second.
    // The GPU time uses timestamp queries, which are read a few frames later. Reporting is skipped if k_Enabled is false.
    class FrameProfiler
    {
    private:
        using steady_clock = std::chrono::steady_clock;

        using LogsService = Service::Logs::System::LogsService;

        using Cpu = Viewer::Profiler::Type::Cpu;
        using Gpu = Viewer::Profiler::Type::Gpu;
        using Counter = Viewer::Profiler::Type::Counter;
        using Slot = Viewer::Profiler::Type::Slot;

    public:
        // Compile-time switch. When false, every method does nothing.
        static constexpr bool k_Enabled{ true };

        FrameProfiler() = default;
        ~FrameProfiler() = default;

        FrameProfiler(const FrameProfiler&) = delete;
        auto operator=(const FrameProfiler&) -> FrameProfiler& = delete;

        // Starts the measuring window on the first call, creates the GPU queries and collects the finished ones.
        auto BeginFrame(ID3D11Device* device, ID3D11DeviceContext* context) -> void;

        // Adds the time of a CPU section. The Frame section also counts the frame.
        auto AddCpu(Cpu slot, double milliseconds) -> void;
        auto SetCounter(Counter counter, std::uint32_t value) -> void;

        // Starts the GPU timing of a frame. Skipped if the queries failed or the slot still has pending results.
        auto BeginGpu(ID3D11DeviceContext* context) -> void;

        // Closes a GPU segment at this point of the command stream.
        auto MarkGpu(ID3D11DeviceContext* context, Gpu segment) -> void;

        // Ends the GPU timing of the frame and moves to the next slot.
        auto EndGpu(ID3D11DeviceContext* context) -> void;

        // Logs the averages and resets the window if at least one second has passed.
        auto Report(LogsService& logsService) -> void;

        // Releases the GPU queries and resets the window. Call it before the device is released.
        auto Release() -> void;

    private:
        // Frames in flight. Each one has its own queries.
        static constexpr std::size_t k_SlotCount{ 4 };

        std::array<Slot, k_SlotCount> m_Slots{};
        std::size_t m_Cursor{ 0 };
        bool m_IsQueriesCreated{ false };
        bool m_IsGpuFailed{ false };
        bool m_IsGpuActive{ false };

        std::array<double, k_CpuCount> m_CpuSum{};
        std::array<double, k_CpuCount> m_CpuMax{};
        std::array<double, k_GpuCount> m_GpuSum{};
        std::array<std::uint32_t, k_CounterCount> m_Counters{};
        std::uint32_t m_Frames{ 0 };
        std::uint32_t m_GpuSamples{ 0 };

        steady_clock::time_point m_WindowStart{};
        bool m_IsWindowStarted{ false };

        auto CreateQueries(ID3D11Device* device) -> void;

        // Reads the finished queries and adds their GPU times to the window.
        auto Collect(ID3D11DeviceContext* context) -> void;

        // Clears the window sums and starts a new window.
        auto Reset() -> void;
    };

    // Measures the time between its construction and its destruction, and adds it to a CPU section.
    class CpuTimer
    {
    private:
        using steady_clock = std::chrono::steady_clock;

        using Cpu = Viewer::Profiler::Type::Cpu;

    public:
        CpuTimer(FrameProfiler& profiler, Cpu slot) :
            m_Profiler(profiler), m_Slot(slot),
            m_Start(FrameProfiler::k_Enabled ? steady_clock::now() : steady_clock::time_point{}) {}

        ~CpuTimer()
        {
            if (!FrameProfiler::k_Enabled) return;

            const auto elapsed = steady_clock::now() - m_Start;
            m_Profiler.AddCpu(m_Slot, std::chrono::duration<double, std::milli>(elapsed).count());
        }

        CpuTimer(const CpuTimer&) = delete;
        auto operator=(const CpuTimer&) -> CpuTimer& = delete;

    private:
        FrameProfiler& m_Profiler;
        Cpu m_Slot;
        steady_clock::time_point m_Start;
    };
}