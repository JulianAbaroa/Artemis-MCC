#include "pch.h"

// Header.
#include "UI_PlayerTable.h"

// --- States ---
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"

// Player.
#include "Core/states/Domain/Player/State_PlayerTable.h"

// --- Systems ---
#include "Core/Systems/Core_System.h"

// Debug.
#include "Core/Systems/Interface/System_Debug.h"

// ImGui.
#include "External/imgui/imgui.h"

void UI_PlayerTable::Draw()
{
	bool hasMapChanged = g_pState->Domain->PlayerTable->HasMapChanged();
	if (hasMapChanged)
	{
		m_CachePlayers = g_pState->Domain->PlayerTable->GetPlayerTable();
		g_pState->Domain->PlayerTable->SetMapChanged(false);
	}

	ImGui::TextDisabled("Live Players Count: %d", (int)m_CachePlayers.size());
	ImGui::Separator();

    m_SearchFilter.DrawSearchBar();
    ImGui::Separator();

	if (!ImGui::BeginChild("PlayersRegion"))
	{
		ImGui::EndChild();
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

		constexpr float kCardWidth = 320.0f;
		constexpr float kCardSpacing = 8.0f;

		bool hasNext = n + 1 < filtered.size();
		bool nextFits = ImGui::GetItemRectMax().x + 
            kCardSpacing + kCardWidth < windowRightEdge;

		if (hasNext && nextFits)
			ImGui::SameLine();
	}

	ImGui::EndChild();
}

void UI_PlayerTable::Cleanup()
{
	m_CachePlayers.clear();
}

void UI_PlayerTable::DrawPlayerCard(const LivePlayer& player)
{
    constexpr float kCardWidth = 320.0f;
    constexpr float kCardHeight = 540.0f;

    ImGui::PushID(player.Handle);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);

    if (ImGui::BeginChild(player.Handle, ImVec2(kCardWidth, kCardHeight),
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
	if (player.Gamertag.empty()) ImGui::TextUnformatted("(no gamertag)");
	else ImGui::TextUnformatted(player.Gamertag.c_str());
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
    char buf[64];

    ImGui::Indent(5.0f);

    snprintf(buf, sizeof(buf), "0x%08X", player.Handle);
    m_CopyableField.Draw("Handle:", buf, player.Handle);

    snprintf(buf, sizeof(buf), "0x%012llX", player.Address);
    m_CopyableField.Draw("Address:", buf, player.Handle);

    this->DrawConnectionStatus(player);
    this->DrawAliveStatus(player);

    ImGui::Unindent(5.0f);
}

void UI_PlayerTable::DrawConnectionStatus(const LivePlayer& player)
{
    const char* label = "Unknown";
    ImVec4 color = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

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
    }

    ImGui::Text("Status:");
    ImGui::SameLine();
    ImGui::TextColored(color, "%s (0x%02X)", label, player.ConnectionState);
}

void UI_PlayerTable::DrawAliveStatus(const LivePlayer& player)
{
    ImGui::Text("State:");
    ImGui::SameLine();

    if (player.IsAlive)
        ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "Alive");
    else
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Dead");
}

void UI_PlayerTable::DrawCardWeapon(const LivePlayer& player)
{
    char buf[64];

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Weapon");
    ImGui::Separator();
    ImGui::Indent(5.0f);

    auto drawHandle = [&](const char* label, uint32_t handle) {
        if (handle == 0xFFFFFFFF)
        {
            ImGui::TextDisabled("none", label);
        }
        else
        {
            snprintf(buf, sizeof(buf), "0x%08X", handle);
            m_CopyableField.Draw(label, buf, 
                player.Handle ^ (uint32_t)(uintptr_t)label);
        }
    };

    drawHandle("Primary Weapon:", player.PrimaryWeaponHandle);
    drawHandle("Secondary Weapon:", player.SecondaryWeaponHandle);
    drawHandle("Objective:", player.ObjectiveHandle);

    ImGui::Spacing();
    ImGui::Text("Position: %.3f, %.3f, %.3f",
        player.WeaponPosition[0], player.WeaponPosition[1], player.WeaponPosition[2]);
    ImGui::Text("Forward: %.3f, %.3f, %.3f",
        player.WeaponForward[0], player.WeaponForward[1], player.WeaponForward[2]);
    ImGui::Text("Aim Offset: %.3f, %.3f, %.3f",
        player.AimOffset[0], player.AimOffset[1], player.AimOffset[2]);

    ImGui::Unindent(5.0f);
}

void UI_PlayerTable::DrawCardBiped(const LivePlayer& player)
{
    char buf[64]{};

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Biped");
    ImGui::Separator();
    ImGui::Indent(5.0f);

    auto drawHandle = [&](const char* label, uint32_t handle) {
        if (handle == 0xFFFFFFFF)
        {
            ImGui::TextDisabled("none", label);
        }
        else
        {
            snprintf(buf, sizeof(buf), "0x%08X", handle);
            m_CopyableField.Draw(label, buf, 
                player.Handle ^ (uint32_t)(uintptr_t)label);
        }
    };

    drawHandle("Current Biped:", player.CurrentBipedHandle);
    drawHandle("Alive Biped:", player.AliveBipedHandle);
    drawHandle("Dead Biped:", player.DeadBipedHandle);

    ImGui::Unindent(5.0f);
}