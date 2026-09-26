export module UI.PlayerTable.System;

import Export.Tick.Type;
import Export.Tick.State;
import UI.PlayerTable.State;
import UI.Widget.System;
import std;

export namespace UI::PlayerTable::System
{
	class PlayerTableUIService
	{
	private:
		using AlivePlayer = Export::Tick::Type::AlivePlayer;

		using TickStore = Export::Tick::State::TickStore;
		using PlayerTableUIStore = UI::PlayerTable::State::PlayerTableUIStore;
		using SearchFilterUIService = UI::Widget::System::SearchFilterUIService;
		using CopyableFieldUIService = UI::Widget::System::CopyableFieldUIService;
		using ResponsiveCardUIService = UI::Widget::System::ResponsiveCardUIService;

	public:
		PlayerTableUIService(TickStore& tickStore, PlayerTableUIStore& playerTableUIStore) :
			m_TickStore(tickStore), m_PlayerTableUIStore(playerTableUIStore) {
		}
		~PlayerTableUIService() = default;

		PlayerTableUIService(const PlayerTableUIService&) = delete;
		PlayerTableUIService& operator=(const PlayerTableUIService&) = delete;

		auto Draw() -> void;

	private:
		TickStore& m_TickStore;
		PlayerTableUIStore& m_PlayerTableUIStore;

		SearchFilterUIService m_SearchFilter{};
		CopyableFieldUIService m_CopyableField{};

		auto RefreshSnapshot() -> void;
		auto PassesFilter(const AlivePlayer& player) const -> bool;

		auto DrawPlayerCard(const AlivePlayer& player) -> void;
		auto DrawCardHeader(const AlivePlayer& player) -> void;
		auto DrawCardIdentity(const AlivePlayer& player) -> void;
		auto DrawConnectionStatus(const AlivePlayer& player) -> void;
		auto DrawCardWeapon(const AlivePlayer& player) -> void;
		auto DrawCardBiped(const AlivePlayer& player) -> void;
	};
}