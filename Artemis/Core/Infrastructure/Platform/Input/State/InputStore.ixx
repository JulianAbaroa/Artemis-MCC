export module Platform.Input.State:Input;

import Platform.Input.Type;
import std;

export namespace Platform::Input::State
{
	class InputStore
	{
	private:
		using Action = Platform::Input::Type::Action;

		static constexpr std::size_t k_MaxButtonId = 256;
		static constexpr std::uint16_t k_MinHoldTicks = 2;

	public:
		InputStore() = default;
		~InputStore() = default;

		auto SetActionRequested(Action action, bool requested) -> void;

		auto AdvanceTick() -> void;

		auto IsActionHeld(short buttonID) const -> bool;

		auto Cleanup() -> void;

	private:
		std::array<std::atomic<bool>, k_MaxButtonId> m_Requested{};
		std::array<std::atomic<std::uint16_t>, k_MaxButtonId> m_TicksRemaining{};

		static auto ToIndex(short buttonID) -> std::size_t;
		static auto ToIndex(Action action) -> std::size_t;
	};
}