#include "pch.h"

#include "UI_PlayerTable.h"

#include "Core/states/Tables/Player/State_PlayerTable.h"

#include "Core/UI/Utils/Handle/HandleDrawer.h"
#include "Core/UI/Utils/String/EnumToString.h"
#include "Core/UI/Utils/Color/TeamToColor.h"
#include "Core/UI/Utils/Hex/HexFormater.h"

#include "External/imgui/imgui.h"

#include <format>

void UI_PlayerTable::Draw()
{
    if (!m_Visible) return;

    this->FirstDraw();
    this->ApplyResetIfRequested("Player Table");

    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (UI_Tab::s_Locked) flags |= ImGuiWindowFlags_NoMove;

    ImGui::Begin("Player Table", &m_Visible, flags);

	if (State_PlayerTable.HasMapChanged())
	{
		m_CachePlayers = State_PlayerTable.GetPlayerTable();
        State_PlayerTable.SetMapChanged(false);
	}

	ImGui::TextDisabled("Live Players Count: %d", 
        (int)m_CachePlayers.size());

	ImGui::Separator();

    m_SearchFilter.DrawSearchBar();

    ImGui::Separator();

	if (!ImGui::BeginChild("Players Region"))
	{
		ImGui::EndChild();
        ImGui::End();
		return;
	}

	float windowRightEdge = ImGui::GetWindowPos().x + 
        ImGui::GetWindowContentRegionMax().x;

    std::vector<const LivePlayer*> filtered;
    filtered.reserve(m_CachePlayers.size());

    for (const auto& [handle, player] : m_CachePlayers)
    {
        if (m_SearchFilter.PassesFilter(player))
        { 
            filtered.push_back(&player);
        }
    }

	for (size_t n = 0; n < filtered.size(); n++)
	{
		this->DrawPlayerCard(*filtered[n]);

		bool hasNext = n + 1 < filtered.size();
		bool nextFits = ImGui::GetItemRectMax().x + 
            328.0f < windowRightEdge;

        if (hasNext && nextFits) ImGui::SameLine();
	}

	ImGui::EndChild();

    ImGui::End();
}

void UI_PlayerTable::DrawPlayerCard(const LivePlayer& player)
{
    ImGui::PushID(player.Handle);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

    if (ImGui::BeginChild(player.Handle, ImVec2(320.0f, 480.0f),
        true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        this->DrawCardHeader(player);
        this->DrawCardIdentity(player);
        this->DrawCardWeapon(player);
        this->DrawCardBiped(player);
    }

    ImGui::EndChild();
    ImGui::PopStyleVar();
    ImGui::PopID();
}

void UI_PlayerTable::DrawCardHeader(const LivePlayer& player)
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));

    if (player.Gamertag.empty())
    {
        ImGui::TextUnformatted("(no gamertag)");
    }
    else
    {
        ImGui::TextUnformatted(player.Gamertag.c_str());
    }

	ImGui::PopStyleColor();

	if (!player.Tag.empty())
	{
		ImGui::SameLine();
		ImGui::TextDisabled("[%s]", player.Tag.c_str());
	}

	ImGui::Separator();
}

void UI_PlayerTable::DrawCardIdentity(const LivePlayer& player)
{
    ImGui::Indent(5.0f);

    m_CopyableField.Draw("Handle:", HexFormater::Hex32(player.Handle), player.Handle);
    m_CopyableField.Draw("Address:", HexFormater::Hex64(player.Address), player.Handle);

    ImGui::Text("Team:");
    ImGui::SameLine();
    ImGui::TextColored(TeamToColor::TeamColorVec4(player.Team, 1.0f), 
        "%s", EnumToString::TeamToString(player.Team));

    this->DrawConnectionStatus(player);

    ImGui::Text("State:");
    ImGui::SameLine();

    if (player.IsAlive)
    {
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "Alive");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Dead");
    }

    ImGui::Unindent(5.0f);
}

void UI_PlayerTable::DrawConnectionStatus(const LivePlayer& player)
{
    const char* label;
    ImVec4 color;

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
        label = "Unknown";
        color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
        break;
    }

    ImGui::Text("Status:");

    ImGui::SameLine();

    ImGui::TextColored(color, "%s (%s)", label, HexFormater::Hex8(
        (uint8_t)player.ConnectionState).c_str());
}

void UI_PlayerTable::DrawCardWeapon(const LivePlayer& player)
{
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Weapon");
    ImGui::Separator();
    ImGui::Indent(5.0f);

    HandleDrawer::DrawU32("Primary Weapon:", player.PrimaryWeaponHandle,
        player, m_CopyableField);

    HandleDrawer::DrawU32("Secondary Weapon:", player.SecondaryWeaponHandle,
        player, m_CopyableField);

    HandleDrawer::DrawU32("Objective:", player.ObjectiveHandle, 
        player, m_CopyableField);

    ImGui::Unindent(5.0f);
}

void UI_PlayerTable::DrawCardBiped(const LivePlayer& player)
{
    ImGui::Spacing();

    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Biped");

    ImGui::Separator();

    ImGui::Indent(5.0f);

    HandleDrawer::DrawU32("Current Biped:", player.CurrentBipedHandle, 
        player, m_CopyableField);

    HandleDrawer::DrawU32("Alive Biped:", player.AliveBipedHandle,
        player, m_CopyableField);

    HandleDrawer::DrawU32("Dead Biped:", player.DeadBipedHandle,
        player, m_CopyableField);

    ImGui::Unindent(5.0f);
}

void UI_PlayerTable::Cleanup()
{
    m_CachePlayers.clear();
}