module;

#include <d3d11.h>

module Viewer.Profiler.System;

import std;

namespace
{
    using std::chrono::duration;

    using Viewer::Profiler::Type::k_GpuCount;
    using Viewer::Profiler::Type::k_CpuCount;

    constexpr std::array<const char*, k_CpuCount> k_CpuNames{
        "scene", "dynamic", "zone", "raycast", "aim", "draw(submit)", "FRAME" };
    constexpr std::array<const char*, k_GpuCount> k_GpuNames{
        "clear", "map", "dynamic", "zone", "raycast", "aim" };
}

namespace Viewer::Profiler::System
{
    auto FrameProfiler::CreateQueries(ID3D11Device* device) -> void
    {
        m_IsQueriesCreated = true;

        D3D11_QUERY_DESC disjointDesc{};
        disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;

        D3D11_QUERY_DESC stampDesc{};
        stampDesc.Query = D3D11_QUERY_TIMESTAMP;

        for (Slot& slot : m_Slots)
        {
            if (FAILED(device->CreateQuery(&disjointDesc, slot.Disjoint.GetAddressOf())))
            {
                m_IsGpuFailed = true;
                return;
            }

            for (auto& stamp : slot.Stamps)
            {
                if (FAILED(device->CreateQuery(&stampDesc, stamp.GetAddressOf())))
                {
                    m_IsGpuFailed = true;
                    return;
                }
            }
        }
    }

    auto FrameProfiler::BeginFrame(ID3D11Device* device, ID3D11DeviceContext* context) -> void
    {
        if (!k_Enabled) return;

        if (!m_IsWindowStarted)
        {
            m_WindowStart = steady_clock::now();
            m_IsWindowStarted = true;
        }

        if (!m_IsQueriesCreated && device) this->CreateQueries(device);

        if (!m_IsGpuFailed && context) this->Collect(context);
    }

    auto FrameProfiler::AddCpu(Cpu slot, double milliseconds) -> void
    {
        if (!k_Enabled) return;

        const auto index = static_cast<std::size_t>(slot);

        m_CpuSum[index] += milliseconds;
        m_CpuMax[index] = (std::max)(m_CpuMax[index], milliseconds);

        if (slot == Cpu::Frame) ++m_Frames;
    }

    auto FrameProfiler::SetCounter(Counter counter, std::uint32_t value) -> void
    {
        if (!k_Enabled) return;

        m_Counters[static_cast<std::size_t>(counter)] = value;
    }

    auto FrameProfiler::BeginGpu(ID3D11DeviceContext* context) -> void
    {
        if (!k_Enabled) return;

        m_IsGpuActive = false;

        Slot& slot = m_Slots[m_Cursor];
        if (m_IsGpuFailed || !context || slot.Pending) return;

        context->Begin(slot.Disjoint.Get());
        context->End(slot.Stamps[0].Get());
        m_IsGpuActive = true;
    }

    auto FrameProfiler::MarkGpu(ID3D11DeviceContext* context, Gpu segment) -> void
    {
        if (!k_Enabled || !m_IsGpuActive) return;

        context->End(m_Slots[m_Cursor].Stamps[static_cast<std::size_t>(segment) + 1].Get());
    }

    auto FrameProfiler::EndGpu(ID3D11DeviceContext* context) -> void
    {
        if (!k_Enabled || !m_IsGpuActive) return;

        Slot& slot = m_Slots[m_Cursor];

        context->End(slot.Disjoint.Get());
        slot.Pending = true;

        m_Cursor = (m_Cursor + 1) % k_SlotCount;
        m_IsGpuActive = false;
    }

    auto FrameProfiler::Collect(ID3D11DeviceContext* context) -> void
    {
        for (Slot& slot : m_Slots)
        {
            if (!slot.Pending) continue;

            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint{};
            if (context->GetData(slot.Disjoint.Get(), &disjoint, sizeof(disjoint),
                D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
            {
                continue;
            }

            std::array<UINT64, k_GpuCount + 1> stamps{};
            bool ready = true;

            for (std::size_t i = 0; i < stamps.size(); ++i)
            {
                if (context->GetData(slot.Stamps[i].Get(), &stamps[i], sizeof(UINT64),
                    D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
                {
                    ready = false;
                    break;
                }
            }

            if (!ready) continue;

            slot.Pending = false;
            if (disjoint.Disjoint || disjoint.Frequency == 0) continue;

            for (std::size_t i = 0; i < k_GpuCount; ++i)
            {
                m_GpuSum[i] += static_cast<double>(stamps[i + 1] - stamps[i]) * 1000.0 /
                    static_cast<double>(disjoint.Frequency);
            }

            ++m_GpuSamples;
        }
    }

    auto FrameProfiler::Report(LogsService& logsService) -> void
    {
        if (!k_Enabled || !m_IsWindowStarted) return;

        const auto now = steady_clock::now();
        const double seconds = duration<double>(now - m_WindowStart).count();
        if (seconds < 1.0 || m_Frames == 0) return;

        const double frames = static_cast<double>(m_Frames);

        std::string text = std::format("[FrameProfiler] INFO: {:.0f} fps | CPU ms avg/max:",
            frames / seconds);

        for (std::size_t i = 0; i < k_CpuCount; ++i)
        {
            text += std::format(" {} {:.2f}/{:.2f};", k_CpuNames[i],
                m_CpuSum[i] / frames, m_CpuMax[i]);
        }

        if (m_GpuSamples > 0)
        {
            const double samples = static_cast<double>(m_GpuSamples);
            double total = 0.0;

            text += " | GPU ms avg:";
            for (std::size_t i = 0; i < k_GpuCount; ++i)
            {
                const double average = m_GpuSum[i] / samples;
                total += average;
                text += std::format(" {} {:.2f};", k_GpuNames[i], average);
            }

            text += std::format(" TOTAL {:.2f}", total);
        }
        else
        {
            text += " | GPU: n/a";
        }

        text += std::format(" | count dynInst={} dynDraws={} aimInst={}",
            m_Counters[static_cast<std::size_t>(Counter::DynamicInstances)],
            m_Counters[static_cast<std::size_t>(Counter::DynamicDraws)],
            m_Counters[static_cast<std::size_t>(Counter::AimInstances)]);
        text += ".";

        //logsService.Message("{}", text);

        this->Reset();
    }

    auto FrameProfiler::Reset() -> void
    {
        m_CpuSum.fill(0.0);
        m_CpuMax.fill(0.0);
        m_GpuSum.fill(0.0);
        m_Frames = 0;
        m_GpuSamples = 0;
        m_WindowStart = steady_clock::now();
    }

    auto FrameProfiler::Release() -> void
    {
        for (Slot& slot : m_Slots)
        {
            slot.Disjoint.Reset();
            for (auto& stamp : slot.Stamps) stamp.Reset();
            slot.Pending = false;
        }

        m_Cursor = 0;
        m_IsQueriesCreated = false;
        m_IsGpuFailed = false;
        m_IsGpuActive = false;
        m_IsWindowStarted = false;

        this->Reset();
    }
}