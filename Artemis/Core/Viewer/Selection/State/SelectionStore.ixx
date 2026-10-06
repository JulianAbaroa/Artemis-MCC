export module Viewer.Selection.State;

import std;

export namespace Viewer::Selection::State
{
    // Handle that means no object is selected.
    constexpr std::uint32_t k_NoSelection{ 0xFFFFFFFF };

    // Holds the selected object handle and the pending pick request.
    // The values are atomic because the input writes them and the render frame reads them.
    class SelectionStore
    {
    public:
        SelectionStore() = default;
        ~SelectionStore() = default;

        SelectionStore(const SelectionStore&) = delete;
        auto operator=(const SelectionStore&) -> SelectionStore& = delete;

        auto GetSelected() const -> std::uint32_t;
        auto SetSelected(std::uint32_t handle) -> void;
        auto HasSelection() const -> bool;
        auto Clear() -> void;

        // Asks the selection service to pick the object under the ray on its next update.
        auto RequestPick() -> void;

        // return: True once per request.
        auto ConsumePick() -> bool;

    private:
        std::atomic<std::uint32_t> m_Selected{ k_NoSelection };
        std::atomic<bool> m_IsPickPending{ false };
    };
}