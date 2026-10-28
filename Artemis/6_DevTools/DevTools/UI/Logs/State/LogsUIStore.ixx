export module UI.Logs.State;

import std;

export namespace UI::Logs::State
{
    // Search text, line selection and copy animation of the logs window.
    class LogsUIStore
    {
    public:
        // Marks that no line is selected or animated.
        static constexpr int k_NoIndex{ -1 };

        // Marks that the copy animation covers the whole selection.
        static constexpr int k_SelectionIndex{ -2 };

        LogsUIStore() = default;
        ~LogsUIStore() = default;

        LogsUIStore(const LogsUIStore&) = delete;
        auto operator=(const LogsUIStore&) -> LogsUIStore& = delete;

        // Returns the text buffer the search input edits.
        auto GetSearchBuffer() -> char*;

        // Returns the capacity of the search buffer.
        auto GetSearchBufferSize() const -> std::size_t;

        // Returns the current search text.
        auto GetSearchText() const -> std::string_view;

        // Clears the search text.
        auto ClearSearch() -> void;

        // Checks whether a range of lines is selected.
        auto HasSelection() const -> bool;

        // Checks whether the line is inside the selection.
        auto IsIndexSelected(int index) const -> bool;

        // Selects only one line.
        auto SelectSingle(int index) -> void;

        // Extends the selection up to the line.
        // note: Selects only that line if there was no selection.
        auto ExtendSelection(int index) -> void;

        // Clears the selection.
        auto ClearSelection() -> void;

        // Returns the first selected line.
        auto GetSelectionMin() const -> int;

        // Returns the last selected line.
        auto GetSelectionMax() const -> int;

        // Starts the copy animation of a line or of the selection.
        // param startTime: UI time in seconds.
        auto StartCopyAnimation(int index, float startTime) -> void;

        // Stops the copy animation.
        auto StopCopyAnimation() -> void;

        // Returns the animated line, k_NoIndex when there is none.
        auto GetAnimatedIndex() const -> int;

        // Returns the UI time in seconds when the animation started.
        auto GetAnimationStartTime() const -> float;

    private:
        char m_SearchBuffer[128]{};

        int m_SelectionStart{ k_NoIndex };
        int m_SelectionEnd{ k_NoIndex };

        int m_AnimatedIndex{ k_NoIndex };
        float m_AnimationStartTime{ 0.0f };
    };
}