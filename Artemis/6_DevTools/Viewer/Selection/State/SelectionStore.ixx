export module Viewer.Selection.State;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace Viewer::Selection::State
{
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
        std::atomic<std::uint32_t> m_Selected{ k_NoHandle };
        std::atomic<bool> m_IsPickPending{ false };
    };
}