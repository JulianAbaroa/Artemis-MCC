export module DevTools.Workbench.PlayerTable.System;

import Engine.Capture.State;
import Blam.Runtime.Player.Type;
import Bridge.Codec.Tick.Type;
import Bridge.Codec.Tick.State;
import DevTools.Gui.Widget.System;
import DevTools.Workbench.PlayerTable.Type;
import DevTools.Workbench.PlayerTable.State;
import std;

export namespace DevTools::Workbench::PlayerTable::System
{
    // Draws the live players as cards with a search filter.
    class PlayerTableUIService
    {
    private:
        using AlivePlayer = Blam::Runtime::Player::Type::Alive::AlivePlayer;

        using CaptureStore = Engine::Capture::State::CaptureStore;

        using TickStore = Bridge::Codec::Tick::State::TickStore;

        using SearchFilterGuiService = DevTools::Gui::Widget::System::SearchFilterGuiService;
        using CopyableFieldGuiService = DevTools::Gui::Widget::System::CopyableFieldGuiService;
        using ResponsiveCardGuiService = DevTools::Gui::Widget::System::ResponsiveCardGuiService;

        using Players = DevTools::Workbench::PlayerTable::Type::Players;
        using PlayerTableUIStore = DevTools::Workbench::PlayerTable::State::PlayerTableUIStore;

    public:
        PlayerTableUIService(CaptureStore& captureStore, TickStore& tickStore, PlayerTableUIStore& playerTableUIStore) :
            m_CaptureStore(captureStore), m_TickStore(tickStore), m_PlayerTableUIStore(playerTableUIStore) {}
        ~PlayerTableUIService() = default;

        PlayerTableUIService(const PlayerTableUIService&) = delete;
        auto operator=(const PlayerTableUIService&) -> PlayerTableUIService& = delete;

        // Draws the player table window content.
        auto Draw() -> void;

    private:
        CaptureStore& m_CaptureStore;
        TickStore& m_TickStore;
        PlayerTableUIStore& m_PlayerTableUIStore;

        SearchFilterGuiService m_SearchFilter{};
        CopyableFieldGuiService m_CopyableField{};

        // Rebuilds the player list when the tick has a new player table and takes the player array address from the published snapshot (D196).
        auto RefreshSnapshot() -> void;

        // Checks whether the player matches the search text.
        auto PassesFilter(const AlivePlayer& player) const -> bool;

        // Draws the card of a player.
        auto DrawPlayerCard(const AlivePlayer& player) -> void;

        // Draws the gamertag and the clan tag of the card.
        auto DrawCardHeader(const AlivePlayer& player) -> void;

        // Draws the handle, the address, the team and the connection status.
        auto DrawCardIdentity(const AlivePlayer& player) -> void;

        // Draws the connection status with its raw value.
        auto DrawConnectionStatus(const AlivePlayer& player) -> void;

        // Draws the weapon and objective handles.
        auto DrawCardWeapon(const AlivePlayer& player) -> void;

        // Draws the biped handles.
        auto DrawCardBiped(const AlivePlayer& player) -> void;
    };
}