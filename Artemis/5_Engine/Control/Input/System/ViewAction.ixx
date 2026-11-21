export module Engine.Control.Input.System:ViewAction;

import Common.Engine.Type;
import Blam.Layout.Action.Type;
import std;

namespace
{
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_HandleIndexMask;

    using Blam::Layout::Action::Type::Size::k_MaxActionPlayers;
}

export namespace Engine::Control::Input::System
{
    // Decides whether the action of the tick must be copied into the input of a view, as the engine does in the theater (VW.9a.y, D349).
    // param viewPlayer: Player handle the view shows, or k_NoHandle without one.
    // param updateMask: Players with an action in the update (after the overrides).
    // param inputUsers: Input user of every player by table index, or -1 when the player is not local.
    // param isTheater: The engine already copies the action itself.
    // return: Table index of the player whose action goes to the view, or nullopt. Never a local player: 0x663bc already moves his view
    // every frame.
    constexpr auto FindViewActionPlayer(std::uint32_t viewPlayer, std::uint32_t updateMask, std::span<const std::int32_t, k_MaxActionPlayers> inputUsers,
        bool isTheater) -> std::optional<std::size_t>
    {
        if (isTheater || viewPlayer == k_NoHandle) return std::nullopt;

        // note: By table index, as 0x47050 itself reads the mask and the action of the view's player. The salt is not checked: a player that
        // leaves is taken out of every view by the engine (0x53e70), and the mask bit says the slot plays this tick.
        const std::size_t player{ viewPlayer & k_HandleIndexMask };
        if (player >= k_MaxActionPlayers) return std::nullopt;
        if ((updateMask & (1u << player)) == 0) return std::nullopt;
        if (inputUsers[player] != -1) return std::nullopt;

        return player;
    }
}