module Platform.Input.State;
import :Input;

namespace Platform::Input::State
{
	auto InputStore::ToIndex(short buttonID) -> std::size_t
	{
		return static_cast<std::size_t>(static_cast<unsigned short>(buttonID));
	}

	auto InputStore::ToIndex(Action action) -> std::size_t
	{
		return ToIndex(static_cast<short>(action));
	}

	auto InputStore::SetActionRequested(Action action, bool requested) -> void
	{
		const auto index = this->ToIndex(action);
		if (index >= k_MaxButtonId) return;

		m_Requested[index].store(requested, std::memory_order_release);

		if (!requested) return;

		auto current = m_TicksRemaining[index].load(std::memory_order_relaxed);
		while (current < k_MinHoldTicks && !m_TicksRemaining[index].
			compare_exchange_weak(current, k_MinHoldTicks, std::memory_order_relaxed)) {}
	}

	auto InputStore::AdvanceTick() -> void
	{
		for (std::size_t index = 0; index < k_MaxButtonId; ++index)
		{
			if (m_Requested[index].load(std::memory_order_relaxed))
			{
				m_TicksRemaining[index].store(k_MinHoldTicks, std::memory_order_relaxed);
				continue;
			}

			const auto remaining = m_TicksRemaining[index].load(std::memory_order_relaxed);
			if (remaining > 0)
			{
				m_TicksRemaining[index].store(
					static_cast<std::uint16_t>(remaining - 1), std::memory_order_relaxed);
			}
		}
	}

	auto InputStore::IsActionHeld(short buttonID) const -> bool
	{
		const auto index = this->ToIndex(buttonID);
		if (index >= k_MaxButtonId) return false;

		return m_Requested[index].load(std::memory_order_acquire) ||
			m_TicksRemaining[index].load(std::memory_order_acquire) > 0;
	}

	auto InputStore::Cleanup() -> void
	{
		for (std::size_t index = 0; index < k_MaxButtonId; ++index)
		{
			m_Requested[index].store(false, std::memory_order_relaxed);
			m_TicksRemaining[index].store(0, std::memory_order_relaxed);
		}
	}
}