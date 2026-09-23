export module Service.Telemetry.State;

import std;

export namespace Service::Telemetry::State
{
	class TelemetryStore
	{
	public:
		TelemetryStore() = default;
		~TelemetryStore() = default;

		auto RecordDroppedTicks(std::uint64_t dropped) -> void;
		auto RecordSimTick(int ticks, std::uint64_t durationNs) -> void;
		auto RecordTickTime(std::uint64_t durationNs) -> void;
		auto RecordPresent() -> void;

		std::atomic<std::uint64_t> m_Dropped{ 0 };
		std::atomic<std::uint32_t> m_DroppedOut{ 0 };

		std::atomic<std::uint64_t> m_SimCalls{ 0 };
		std::atomic<std::uint64_t> m_Ticks{ 0 };
		std::atomic<std::uint64_t> m_SimNs{ 0 };
		std::atomic<std::uint64_t> m_Sweeps{ 0 };
		std::atomic<std::uint64_t> m_SweepNs{ 0 };
		std::atomic<std::uint64_t> m_Presents{ 0 };

		std::atomic<float> m_TickHz{ 0.0f };
		std::atomic<float> m_PresentHz{ 0.0f };
		std::atomic<float> m_SimMs{ 0.0f };
		std::atomic<float> m_SweepMs{ 0.0f };
	};
}