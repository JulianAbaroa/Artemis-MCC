module Platform.Input.State;
import :MouseDelta;

namespace Platform::Input::State
{
	auto MouseDeltaStore::SetAIControlActive(bool active) -> void
	{
		m_AIControlActive.store(active, std::memory_order_release);
	}

	auto MouseDeltaStore::IsAIControlActive() const -> bool
	{
		return m_AIControlActive.load(std::memory_order_acquire);
	}

	auto MouseDeltaStore::AddPendingDelta(std::int32_t deltaX, std::int32_t deltaY) -> void
	{
		m_PendingDeltaX.fetch_add(deltaX, std::memory_order_relaxed);
		m_PendingDeltaY.fetch_add(deltaY, std::memory_order_relaxed);
	}

	auto MouseDeltaStore::ConsumePendingDelta(std::int32_t& outDeltaX, std::int32_t& outDeltaY) -> bool
	{
		outDeltaX = m_PendingDeltaX.exchange(0, std::memory_order_acq_rel);
		outDeltaY = m_PendingDeltaY.exchange(0, std::memory_order_acq_rel);

		return outDeltaX != 0 || outDeltaY != 0;
	}

	auto MouseDeltaStore::SetKnownDeviceHandle(HANDLE device) -> void
	{
		m_KnownDeviceHandle.store(device, std::memory_order_release);
	}

	auto MouseDeltaStore::GetKnownDeviceHandle() const -> HANDLE
	{
		return m_KnownDeviceHandle.load(std::memory_order_acquire);
	}

	auto MouseDeltaStore::Cleanup() -> void
	{
		m_AIControlActive.store(false, std::memory_order_relaxed);
		m_PendingDeltaX.store(0, std::memory_order_relaxed);
		m_PendingDeltaY.store(0, std::memory_order_relaxed);
		m_KnownDeviceHandle.store(nullptr, std::memory_order_relaxed);
	}
}