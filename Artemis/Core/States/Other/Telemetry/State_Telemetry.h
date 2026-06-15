#pragma once

#include <atomic>
#include <cstdint>

class State_Telemetry
{
public:
	State_Telemetry() = default;
	~State_Telemetry() = default;

	void RecordDroppedTicks(uint64_t dropped);
	void RecordSimTick(int ticks, uint64_t durationNs);
	void RecordAISweep(uint64_t durationNs);
	void RecordPresent();

	std::atomic<uint64_t> m_Dropped{ 0 };
	std::atomic<uint32_t> m_DroppedOut{ 0 };

	std::atomic<uint64_t> m_SimCalls{ 0 };
	std::atomic<uint64_t> m_Ticks{ 0 };
	std::atomic<uint64_t> m_SimNs{ 0 };
	std::atomic<uint64_t> m_Sweeps{ 0 };
	std::atomic<uint64_t> m_SweepNs{ 0 };
	std::atomic<uint64_t> m_Presents{ 0 };

	std::atomic<float> m_TickHz{ 0.0f };
	std::atomic<float> m_PresentHz{ 0.0f };
	std::atomic<float> m_SimMs{ 0.0f };
	std::atomic<float> m_SweepMs{ 0.0f };
};