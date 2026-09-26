module;

#include "External/imgui/imgui.h"

module UI.PlayerTable.System;

import Tables.Player.Type;
import UI.Color.System;
import UI.Format.System;
import std;

namespace
{
	using ConnectionState = Tables::Player::Type::Alive::ConnectionState;

	using TeamToColor = UI::Color::System::TeamToColor;
	using HexFormater = UI::Format::System::HexFormater;
	using TeamFormater = UI::Format::System::TeamFormater;
	using HandleDrawerUIService = UI::Widget::System::HandleDrawerUIService;

	const ImVec2 k_CardSize{ 320.0f, 480.0f };
	constexpr float k_CardSpacing = 8.0f;

	const ImVec4 k_SectionColor{ 1.0f, 0.8f, 0.4f, 1.0f };
}

namespace UI::PlayerTable::System
{
	auto PlayerTableUIService::Draw() -> void
	{
		this->RefreshSnapshot();

		const auto count = static_cast<int>(m_PlayerTableUIStore.GetPlayerCount());
		ImGui::TextDisabled("Live Players Count: %d", count);

		ImGui::Separator();
		m_SearchFilter.DrawSearchBar("Search by gamertag, tag, handle, address...");
		ImGui::Separator();

		if (!ImGui::BeginChild("Players Region"))
		{
			ImGui::EndChild();
			return;
		}

		const float windowRightEdge = ImGui::GetCursorScreenPos().x +
			ImGui::GetContentRegionAvail().x;

		std::vector<const AlivePlayer*> filtered;
		filtered.reserve(m_PlayerTableUIStore.GetPlayers().size());

		for (const AlivePlayer* player : m_PlayerTableUIStore.GetPlayers())
		{
			if (this->PassesFilter(*player)) filtered.push_back(player);
		}

		for (std::size_t i = 0; i < filtered.size(); ++i)
		{
			this->DrawPlayerCard(*filtered[i]);

			const bool hasNext = i + 1 < filtered.size();
			const bool nextFits = ResponsiveCardUIService::FitsOnSameLine(
				k_CardSize.x, k_CardSpacing, windowRightEdge);

			if (hasNext && nextFits) ImGui::SameLine();
		}

		ImGui::EndChild();
	}

	auto PlayerTableUIService::RefreshSnapshot() -> void
	{
		const auto tick = m_TickStore.Acquire();
		auto table = tick ? tick->PlayerTable : nullptr;

		if (m_PlayerTableUIStore.IsSameSnapshot(table)) return;

		if (!table)
		{
			m_PlayerTableUIStore.Cleanup();
			return;
		}

		PlayerTableUIStore::Players players;
		players.reserve(table->size());

		for (const auto& [handle, player] : *table)
		{
			players.push_back(&player);
		}

		std::ranges::sort(players, {}, &AlivePlayer::Handle);

		m_PlayerTableUIStore.SetSnapshot(std::move(table), std::move(players));
	}

	auto PlayerTableUIService::PassesFilter(const AlivePlayer& player) const -> bool
	{
		if (!m_SearchFilter.IsActive()) return true;

		return m_SearchFilter.Matches(player.Gamertag) ||
			m_SearchFilter.Matches(player.Tag) ||
			m_SearchFilter.Matches(HexFormater::Hex32(player.Handle)) ||
			m_SearchFilter.Matches(HexFormater::Hex64(player.Address));
	}

	auto PlayerTableUIService::DrawPlayerCard(const AlivePlayer& player) -> void
	{
		ResponsiveCardUIService::Draw(player.Handle, k_CardSize, [this, &player]
			{
				this->DrawCardHeader(player);
				this->DrawCardIdentity(player);
				this->DrawCardWeapon(player);
				this->DrawCardBiped(player);
			});
	}

	auto PlayerTableUIService::DrawCardHeader(const AlivePlayer& player) -> void
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

		if (player.Gamertag.empty()) ImGui::TextUnformatted("(no gamertag)");
		else                         ImGui::TextUnformatted(player.Gamertag.c_str());

		ImGui::PopStyleColor();

		if (!player.Tag.empty())
		{
			ImGui::SameLine();
			ImGui::TextDisabled("[%s]", player.Tag.c_str());
		}

		ImGui::Separator();
	}

	auto PlayerTableUIService::DrawCardIdentity(const AlivePlayer& player) -> void
	{
		ImGui::Indent(5.0f);

		m_CopyableField.Draw("Handle:", HexFormater::Hex32(player.Handle), player.Handle);
		m_CopyableField.Draw("Address:", HexFormater::Hex64(player.Address), player.Handle);

		ImGui::Text("Team:");
		ImGui::SameLine();
		ImGui::TextColored(TeamToColor::TeamColorVec4(player.Team, 1.0f),
			"%s", TeamFormater::TeamToString(player.Team));

		this->DrawConnectionStatus(player);

		ImGui::Unindent(5.0f);
	}

	auto PlayerTableUIService::DrawConnectionStatus(const AlivePlayer& player) -> void
	{
		const char* label = "Unknown";
		ImVec4 color(0.8f, 0.8f, 0.8f, 1.0f);

		switch (player.ConnectionState)
		{
		case ConnectionState::Connected:
			label = "Connected";
			color = ImVec4(0.2f, 0.9f, 0.4f, 1.0f);
			break;

		case ConnectionState::Disconnected:
			label = "Disconnected";
			color = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
			break;

		case ConnectionState::Connecting:
			label = "Connecting";
			color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
			break;

		default:
			break;
		}

		ImGui::Text("Status:");
		ImGui::SameLine();
		ImGui::TextColored(color, "%s (%s)", label, HexFormater::Hex8(
			static_cast<std::uint8_t>(player.ConnectionState)).c_str());
	}

	auto PlayerTableUIService::DrawCardWeapon(const AlivePlayer& player) -> void
	{
		ImGui::Spacing();
		ImGui::TextColored(k_SectionColor, "Weapon");
		ImGui::Separator();
		ImGui::Indent(5.0f);

		HandleDrawerUIService::DrawU32("Primary Weapon:", player.PrimaryWeaponHandle,
			player.Handle, m_CopyableField);

		HandleDrawerUIService::DrawU32("Secondary Weapon:", player.SecondaryWeaponHandle,
			player.Handle, m_CopyableField);

		HandleDrawerUIService::DrawU32("Objective:", player.ObjectiveHandle,
			player.Handle, m_CopyableField);

		ImGui::Unindent(5.0f);
	}

	auto PlayerTableUIService::DrawCardBiped(const AlivePlayer& player) -> void
	{
		ImGui::Spacing();
		ImGui::TextColored(k_SectionColor, "Biped");
		ImGui::Separator();
		ImGui::Indent(5.0f);

		HandleDrawerUIService::DrawU32("Current Biped:", player.CurrentBipedHandle,
			player.Handle, m_CopyableField);

		HandleDrawerUIService::DrawU32("Alive Biped:", player.AliveBipedHandle,
			player.Handle, m_CopyableField);

		HandleDrawerUIService::DrawU32("Dead Biped:", player.DeadBipedHandle,
			player.Handle, m_CopyableField);

		ImGui::Unindent(5.0f);
	}
}