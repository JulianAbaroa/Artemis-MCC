export module DevTools.Viewer.Selection.State;

import Common.Engine.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
}

export namespace DevTools::Viewer::Selection::State
{
    // Holds the selected object handle, the player it follows and the pending pick and step requests.
    // The values are atomic because the input writes them and the render frame reads them.
    // note: Selecting the biped of a player follows the player (VW.9a B6): the object is the current biped of that player, which the scene service
    // rewrites at every update, and the player stays when the biped dies, respawns or is replaced.
    class SelectionStore
    {
    public:
        SelectionStore() = default;
        ~SelectionStore() = default;

        SelectionStore(const SelectionStore&) = delete;
        auto operator=(const SelectionStore&) -> SelectionStore& = delete;

        auto GetSelected() const -> std::uint32_t;
        // Selects an object, which follows no player. A pick uses it.
        auto SetSelected(std::uint32_t handle) -> void;

        // Selects a player and its current biped (none while it has none).
        auto Follow(std::uint32_t playerHandle, std::uint32_t bipedHandle) -> void;

        // return: The player the selection follows, or 0xFFFFFFFF when the selection is not a player or there is none.
        auto GetSelectedPlayer() const -> std::uint32_t;
        auto HasSelection() const -> bool;
        auto Clear() -> void;

        // Asks the selection service to pick the object under the ray on its next update.
        auto RequestPick() -> void;

        // return: True once per request.
        auto ConsumePick() -> bool;

        // Asks the scene service to move the selected player by delta positions of the roster on its next update (VW.9b). Requests add up.
        // note: Called from the window thread, consumed on the render thread.
        auto RequestStep(int delta) -> void;

        // return: The steps asked since the last call (negative goes back), and clears them.
        auto ConsumeStep() -> int;

    private:
        std::atomic<std::uint32_t> m_Selected{ k_NoHandle };
        std::atomic<std::uint32_t> m_SelectedPlayer{ k_NoHandle };
        std::atomic<bool> m_IsPickPending{ false };
        std::atomic<int> m_PendingSteps{ 0 };
    };
}