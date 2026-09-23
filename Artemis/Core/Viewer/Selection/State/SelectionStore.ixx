export module Viewer.Selection.State;

import std;

export namespace Viewer::Selection::State
{
	constexpr std::uint32_t k_NoSelection = 0xFFFFFFFF;

	class SelectionStore
	{
	public:
		SelectionStore() = default;
		~SelectionStore() = default;

		auto GetSelected() const -> std::uint32_t;
		auto SetSelected(std::uint32_t handle) -> void;
		auto HasSelection() const -> bool;
		auto Clear() -> void;

		auto RequestPick() -> void;
		auto ConsumePick() -> bool;

	private:
		std::atomic<std::uint32_t> m_Selected{ k_NoSelection };
		std::atomic<bool> m_IsPickPending{ false };
	};
}