export module UI.PlayerTable.System;

import Export.Tick.Type;
import Export.Tick.State;
import Gui.Widget.System;
import UI.PlayerTable.Type;
import UI.PlayerTable.State;
import std;

export namespace UI::PlayerTable::System
{
    // Draws the live players as cards with a search filter.
    class PlayerTableUIService
    {
    private:
        using AlivePlayer = Export::Tick::Type::AlivePlayer;

        using TickStore = Export::Tick::State::TickStore;

        using SearchFilterGuiService = Gui::Widget::System::SearchFilterGuiService;
        using CopyableFieldGuiService = Gui::Widget::System::CopyableFieldGuiService;
        using ResponsiveCardGuiService = Gui::Widget::System::ResponsiveCardGuiService;

        using Players = UI::PlayerTable::Type::Players;
        using PlayerTableUIStore = UI::PlayerTable::State::PlayerTableUIStore;

    public:
        PlayerTableUIService(TickStore& tickStore, PlayerTableUIStore& playerTableUIStore) :
            m_TickStore(tickStore), m_PlayerTableUIStore(playerTableUIStore) {}
        ~PlayerTableUIService() = default;

        PlayerTableUIService(const PlayerTableUIService&) = delete;
        auto operator=(const PlayerTableUIService&) -> PlayerTableUIService& = delete;

        // Draws the player table window content.
        auto Draw() -> void;

    private:
        TickStore& m_TickStore;
        PlayerTableUIStore& m_PlayerTableUIStore;

        SearchFilterGuiService m_SearchFilter{};
        CopyableFieldGuiService m_CopyableField{};

        // Rebuilds the player list when the tick has a new player table.
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