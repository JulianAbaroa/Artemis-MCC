module Service.Telemetry.System;

namespace Service::Telemetry::System
{
    auto TelemetryService::Update() -> void
    {
        auto now = Clock::now();

        auto ticksNow = now.time_since_epoch().count();
        auto ticksLast = m_LastUpdate.time_since_epoch().count();

        constexpr double secondsPerTick = static_cast<double>(Clock::period::num) / static_cast<double>(Clock::period::den);
        double seconds = static_cast<double>(ticksNow - ticksLast) * secondsPerTick;

        if (seconds <= 0.0) return;
        m_LastUpdate = now;

        std::uint64_t calls = m_TelemetryStore.m_SimCalls.exchange(0, std::memory_order_relaxed);
        std::uint64_t tickTotal = m_TelemetryStore.m_Ticks.exchange(0, std::memory_order_relaxed);
        std::uint64_t tickNs = m_TelemetryStore.m_SimNs.exchange(0, std::memory_order_relaxed);
        std::uint64_t sweeps = m_TelemetryStore.m_Sweeps.exchange(0, std::memory_order_relaxed);
        std::uint64_t sweepNs = m_TelemetryStore.m_SweepNs.exchange(0, std::memory_order_relaxed);
        std::uint64_t presents = m_TelemetryStore.m_Presents.exchange(0, std::memory_order_relaxed);

        m_TelemetryStore.m_TickHz.store((float)(tickTotal / seconds), std::memory_order_relaxed);
        m_TelemetryStore.m_PresentHz.store((float)(presents / seconds), std::memory_order_relaxed);
        m_TelemetryStore.m_SimMs.store(calls ? (float)((tickNs / (double)calls) / 1e6) : 0.0f, std::memory_order_relaxed);
        m_TelemetryStore.m_SweepMs.store(sweeps ? (float)((sweepNs / (double)sweeps) / 1e6) : 0.0f, std::memory_order_relaxed);

        std::uint64_t dropped = m_TelemetryStore.m_Dropped.exchange(0, std::memory_order_relaxed);
        m_TelemetryStore.m_DroppedOut.store((std::uint32_t)dropped, std::memory_order_relaxed);
    }
}