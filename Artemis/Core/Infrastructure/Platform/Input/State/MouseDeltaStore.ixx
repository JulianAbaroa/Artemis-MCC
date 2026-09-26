module;

#include <windows.h>

export module Platform.Input.State:MouseDelta;

import std;

export namespace Platform::Input::State
{
	class MouseDeltaStore
	{
	public:
		MouseDeltaStore() = default;
		~MouseDeltaStore() = default;

		auto SetAIControlActive(bool active) -> void;
		auto IsAIControlActive() const -> bool;

		auto AddPendingDelta(std::int32_t deltaX, std::int32_t deltaY) -> void;

		auto ConsumePendingDelta(std::int32_t& outDeltaX, std::int32_t& outDeltaY) -> bool;

		auto SetKnownDeviceHandle(HANDLE device) -> void;
		auto GetKnownDeviceHandle() const -> HANDLE;

		auto Cleanup() -> void;

	private:
		std::atomic<bool> m_AIControlActive{ false };

		std::atomic<std::int32_t> m_PendingDeltaX{ 0 };
		std::atomic<std::int32_t> m_PendingDeltaY{ 0 };

		std::atomic<HANDLE> m_KnownDeviceHandle{ nullptr };
	};
}