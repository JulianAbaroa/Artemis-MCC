module;

#include <d3d11.h>
#include <wrl/client.h>

export module Viewer.Profiler.Type;

import std;

export namespace Viewer::Profiler::Type
{
    template <typename T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    // CPU section of the frame. Count is the number of sections and is not a section.
    enum class Cpu : std::size_t { Scene, Dynamic, Zone, Raycast, Aim, Draw, Frame, Count };

    // GPU segment of the frame, measured between two timestamps. Count is not a segment.
    enum class Gpu : std::size_t { Clear, Map, Dynamic, Zone, Raycast, Aim, Count };

    // Value reported with the timings. Count is the number of counters and is not a counter.
    enum class Counter : std::size_t { DynamicInstances, DynamicDraws, AimInstances, Count };

    inline constexpr std::size_t k_CpuCount{ static_cast<std::size_t>(Cpu::Count) };
    inline constexpr std::size_t k_GpuCount{ static_cast<std::size_t>(Gpu::Count) };
    inline constexpr std::size_t k_CounterCount{ static_cast<std::size_t>(Counter::Count) };

    // GPU queries of one frame. The stamps are the start and the end of each Gpu segment.
    // Pending is true from the end of the frame until the results are read.
    struct Slot
    {
        ComPtr<ID3D11Query> Disjoint{};
        std::array<ComPtr<ID3D11Query>, k_GpuCount + 1> Stamps{};
        bool Pending{ false };
    };
}