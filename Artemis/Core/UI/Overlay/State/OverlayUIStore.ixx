export module UI.Overlay.State;

import std;

export namespace UI::Overlay::State
{
	class OverlayUIStore
	{
	public:
		OverlayUIStore() = default;
		~OverlayUIStore() = default;

		OverlayUIStore(const OverlayUIStore&) = delete;
		OverlayUIStore& operator=(const OverlayUIStore&) = delete;

		auto IsVisible() const -> bool;
		auto SetVisible(bool value) -> void;
		auto ToggleVisible() -> void;

	private:
		std::atomic<bool> m_IsVisible{ false };
	};
}