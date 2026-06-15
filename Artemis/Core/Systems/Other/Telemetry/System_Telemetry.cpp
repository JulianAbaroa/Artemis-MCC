#include "pch.h"

#include "System_Telemetry.h"

#include "Core/States/Other/Telemetry/State_Telemetry.h"

void System_Telemetry::Update()
{
	auto now = SteadyClock::now();
	double seconds = Duration_Double(now - m_LastUpdate).count();

	if (seconds <= 0.0) return;
	m_LastUpdate = now;

	auto& telemetry = m_Deps.State_Telemetry;

	uint64_t calls = telemetry.m_SimCalls.exchange(0, std::memory_order_relaxed);
	uint64_t tickTotal = telemetry.m_Ticks.exchange(0, std::memory_order_relaxed);
	uint64_t tickNs = telemetry.m_SimNs.exchange(0, std::memory_order_relaxed);
	uint64_t sweeps = telemetry.m_Sweeps.exchange(0, std::memory_order_relaxed);
	uint64_t sweepNs = telemetry.m_SweepNs.exchange(0, std::memory_order_relaxed);
	uint64_t presents = telemetry.m_Presents.exchange(0, std::memory_order_relaxed);
	
	telemetry.m_TickHz.store((float)(tickTotal / seconds), std::memory_order_relaxed);
	telemetry.m_PresentHz.store((float)(presents / seconds), std::memory_order_relaxed);
	telemetry.m_SimMs.store(calls ? (float)((tickNs / (double)calls) / 1e6) : 0.0f, std::memory_order_relaxed);
	telemetry.m_SweepMs.store(sweeps ? (float)((sweepNs / (double)sweeps) / 1e6) : 0.0f, std::memory_order_relaxed);

	uint64_t dropped = telemetry.m_Dropped.exchange(0, std::memory_order_relaxed);
	telemetry.m_DroppedOut.store((uint32_t)dropped, std::memory_order_relaxed);
}