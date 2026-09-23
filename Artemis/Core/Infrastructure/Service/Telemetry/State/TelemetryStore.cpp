module Service.Telemetry.State;

namespace Service::Telemetry::State
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
	}

	auto TelemetryStore::RecordTickTime(std::uint64_t durationNs) -> void
	{
		m_Sweeps.fetch_add(1, std::memory_order_relaxed);
		m_SweepNs.fetch_add(durationNs, std::memory_order_relaxed);
	}

	auto TelemetryStore::RecordPresent() -> void
	{
		m_Presents.fetch_add(1, std::memory_order_relaxed);
	}
}