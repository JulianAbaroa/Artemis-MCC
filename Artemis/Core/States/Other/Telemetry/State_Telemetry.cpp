#include "pch.h"

#include "State_Telemetry.h"

void State_Telemetry::RecordDroppedTicks(uint64_t dropped)
{
	m_Dropped.fetch_add(dropped, std::memory_order_relaxed);
}

void State_Telemetry::RecordSimTick(int ticks, uint64_t durationNs)
{
	int n = ticks < 0 ? 0 : ticks;
	m_SimCalls.fetch_add(1, std::memory_order_relaxed);
	m_Ticks.fetch_add(n, std::memory_order_relaxed);
	m_SimNs.fetch_add(durationNs, std::memory_order_relaxed);
}

void State_Telemetry::RecordTickTime(uint64_t durationNs)
{
	m_Sweeps.fetch_add(1, std::memory_order_relaxed);
	m_SweepNs.fetch_add(durationNs, std::memory_order_relaxed);
}

void State_Telemetry::RecordPresent()
{
	m_Presents.fetch_add(1, std::memory_order_relaxed);
}