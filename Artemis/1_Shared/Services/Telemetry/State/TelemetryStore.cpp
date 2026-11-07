module Services.Telemetry.State;

import Services.Telemetry.Type;
import std;

namespace
{
    using Services::Telemetry::Type::PerfSnapshot;
    using Services::Telemetry::Type::k_MaxStages;
    using Services::Telemetry::Type::k_SweepBucketCount;
    using Services::Telemetry::Type::k_SweepBucketNs;
}

namespace Services::Telemetry::State
{
    auto TelemetryStore::RecordDroppedTicks(std::uint64_t dropped) -> void
    {
        m_Dropped.fetch_add(dropped, std::memory_order_relaxed);
    }

    auto TelemetryStore::RecordSimTick(int ticks, std::uint64_t durationNs) -> void
    {
        int n = ticks < 0 ? 0 : ticks;
        m_SimCalls.fetch_add(1, std::memory_order_relaxed);
        m_Ticks.fetch_add(n, std::memory_order_relaxed);
        m_SimNs.fetch_add(durationNs, std::memory_order_relaxed);
        m_TotalTicks.fetch_add(n, std::memory_order_relaxed);
        m_TotalEngineNs.fetch_add(durationNs, std::memory_order_relaxed);
    }

    auto TelemetryStore::RecordReadWait(std::uint64_t waitNs, bool isTimedOut) -> void
    {
        m_ReadWaitNs.fetch_add(waitNs, std::memory_order_relaxed);
        m_TotalReadWaitNs.fetch_add(waitNs, std::memory_order_relaxed);
        if (isTimedOut)
        {
            m_ReadTimeouts.fetch_add(1, std::memory_order_relaxed);
            m_TotalReadTimeouts.fetch_add(1, std::memory_order_relaxed);
        }
    }

    auto TelemetryStore::RecordSweep(std::uint64_t durationNs, std::span<const std::uint64_t> stageNs) -> void
    {
        m_Sweeps.fetch_add(1, std::memory_order_relaxed);
        m_SweepNs.fetch_add(durationNs, std::memory_order_relaxed);
        m_TotalSweeps.fetch_add(1, std::memory_order_relaxed);
        m_TotalSweepNs.fetch_add(durationNs, std::memory_order_relaxed);

        // The AI thread is the only writer: a plain compare and store is enough for the maximum.
        if (durationNs > m_SweepMaxNs.load(std::memory_order_relaxed)) m_SweepMaxNs.store(durationNs, std::memory_order_relaxed);

        const auto bucket{ (std::min)(durationNs / k_SweepBucketNs, static_cast<std::uint64_t>(k_SweepBucketCount)) };
        m_SweepHistogram[static_cast<std::size_t>(bucket)].fetch_add(1, std::memory_order_relaxed);

        const std::size_t count{ (std::min)(stageNs.size(), k_MaxStages) };
        for (std::size_t i{ 0 }; i < count; ++i)
        {
            if (stageNs[i] != 0) m_TotalStageNs[i].fetch_add(stageNs[i], std::memory_order_relaxed);
        }
    }

    auto TelemetryStore::RecordLightTick(std::uint64_t durationNs) -> void
    {
        m_LightTicks.fetch_add(1, std::memory_order_relaxed);
        m_LightNs.fetch_add(durationNs, std::memory_order_relaxed);
        m_TotalLightTicks.fetch_add(1, std::memory_order_relaxed);
        m_TotalLightNs.fetch_add(durationNs, std::memory_order_relaxed);
        if (durationNs > m_LightMaxNs.load(std::memory_order_relaxed)) m_LightMaxNs.store(durationNs, std::memory_order_relaxed);
    }

    auto TelemetryStore::RecordObjectTable(std::uint64_t slots, std::uint64_t live) -> void
    {
        if (slots > m_ObjectSlotsMax.load(std::memory_order_relaxed)) m_ObjectSlotsMax.store(slots, std::memory_order_relaxed);
        if (live > m_ObjectsMax.load(std::memory_order_relaxed)) m_ObjectsMax.store(live, std::memory_order_relaxed);
    }

    auto TelemetryStore::SetStageNames(std::span<const std::string_view> names) -> void
    {
        m_StageCount = (std::min)(names.size(), k_MaxStages);
        std::copy_n(names.begin(), m_StageCount, m_StageNames.begin());
    }

    auto TelemetryStore::GetStageNames() const -> std::span<const std::string_view>
    {
        return std::span<const std::string_view>{ m_StageNames.data(), m_StageCount };
    }

    auto TelemetryStore::CopyPerf(PerfSnapshot& out) -> void
    {
        out.Ticks = m_TotalTicks.load(std::memory_order_relaxed);
        out.EngineNs = m_TotalEngineNs.load(std::memory_order_relaxed);
        out.ReadWaitNs = m_TotalReadWaitNs.load(std::memory_order_relaxed);
        out.ReadTimeouts = m_TotalReadTimeouts.load(std::memory_order_relaxed);
        out.Sweeps = m_TotalSweeps.load(std::memory_order_relaxed);
        out.SweepNs = m_TotalSweepNs.load(std::memory_order_relaxed);
        out.SweepMaxNs = m_SweepMaxNs.exchange(0, std::memory_order_relaxed);
        out.LightTicks = m_TotalLightTicks.load(std::memory_order_relaxed);
        out.LightNs = m_TotalLightNs.load(std::memory_order_relaxed);
        out.LightMaxNs = m_LightMaxNs.exchange(0, std::memory_order_relaxed);
        out.ObjectSlotsMax = m_ObjectSlotsMax.exchange(0, std::memory_order_relaxed);
        out.ObjectsMax = m_ObjectsMax.exchange(0, std::memory_order_relaxed);

        for (std::size_t i{ 0 }; i < out.SweepHistogram.size(); ++i) out.SweepHistogram[i] = m_SweepHistogram[i].load(std::memory_order_relaxed);
        for (std::size_t i{ 0 }; i < out.StageNs.size(); ++i) out.StageNs[i] = m_TotalStageNs[i].load(std::memory_order_relaxed);
    }

    auto TelemetryStore::RecordPresent() -> void
    {
        m_Presents.fetch_add(1, std::memory_order_relaxed);
    }
}