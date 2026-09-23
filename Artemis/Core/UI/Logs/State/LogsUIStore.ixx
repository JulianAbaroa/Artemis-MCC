export module UI.Logs.State;

import std;

export namespace UI::Logs::State
{
	class LogsUIStore
	{
	public:
		static constexpr int k_NoIndex = -1;
		static constexpr int k_SelectionIndex = -2;

		LogsUIStore() = default;
		~LogsUIStore() = default;

		LogsUIStore(const LogsUIStore&) = delete;
		LogsUIStore& operator=(const LogsUIStore&) = delete;

		auto GetSearchBuffer() -> char*;
		auto GetSearchBufferSize() const -> std::size_t;
		auto GetSearchText() const -> std::string_view;
		auto ClearSearch() -> void;

		auto HasSelection() const -> bool;
		auto IsIndexSelected(int index) const -> bool;
		auto SelectSingle(int index) -> void;
		auto ExtendSelection(int index) -> void;
		auto ClearSelection() -> void;
		auto GetSelectionMin() const -> int;
		auto GetSelectionMax() const -> int;

		auto StartCopyAnimation(int index, float startTime) -> void;
		auto StopCopyAnimation() -> void;
		auto GetAnimatedIndex() const -> int;
		auto GetAnimationStartTime() const -> float;

	private:
		char m_SearchBuffer[128]{};

		int m_SelectionStart{ k_NoIndex };
		int m_SelectionEnd{ k_NoIndex };

		int m_AnimatedIndex{ k_NoIndex };
		float m_AnimationStartTime{ 0.0f };
	};
}