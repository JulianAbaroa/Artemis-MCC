#include "pch.h"
#include "Core/UI/Domain/Tables/UI_PlayerTable.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/states/Domain/Tables/State_PlayerTable.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/imgui/imgui.h"

void UI_PlayerTable::Draw()
{
	bool hasMapChanged = g_pState->Domain->PlayerTable->HasMapChanged();
	if (hasMapChanged)
	{
		m_CachePlayers = g_pState->Domain->PlayerTable->GetAllPlayers();
		g_pState->Domain->PlayerTable->SetMapChanged(false);
	}

	ImGui::TextDisabled("Live Players Count: %d", (int)m_CachePlayers.size());
	ImGui::Separator();

	if (ImGui::BeginChild("PlayersRegion"))
	{
		float windowVisiblex2 = ImGui::GetWindowPos().x +
			ImGui::GetWindowContentRegionMax().x;

		std::vector<const LivePlayer*> playerList;
		playerList.reserve(m_CachePlayers.size());

		for (const auto& [handle, player] : m_CachePlayers)
		{
			playerList.push_back(&player);
		}

		for (size_t n = 0; n < playerList.size(); n++)
		{
			this->DrawPlayerCard(*playerList[n]);

			float lastCardRight = ImGui::GetItemRectMax().x;
			constexpr float kCardWidth = 320.0f;
			constexpr float kCardSpacing = 8.0f;

			if (n + 1 < playerList.size() &&
				lastCardRight + kCardSpacing + kCardWidth < windowVisiblex2)
			{
				ImGui::SameLine();
			}
		}

		ImGui::EndChild();
	}
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

		auto drawHandle = [](const char* label, uint32_t handle)
			{
				if (handle == 0xFFFFFFFF)
					ImGui::TextDisabled("%-20s none", label);
				else
					ImGui::Text("%-20s 0x%08X", label, handle);
			};

		ImGui::Indent(5.0f);
		ImGui::Text("Handle:  0x%08X", player.Handle);
		ImGui::Text("Address: 0x%012llX", player.Address);

		const char* connectedStr = "Unknown";
		ImVec4      connectedColor = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
		switch (player.ConnectedFlag)
		{
		case 0x01:
			connectedStr = "Connected";
			connectedColor = ImVec4(0.2f, 0.9f, 0.4f, 1.0f);
			break;
		case 0x02:
			connectedStr = "Disconnected";
			connectedColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
			break;
		case 0x08:
			connectedStr = "Connecting";
			connectedColor = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
			break;
		}

		ImGui::Text("Status:");
		ImGui::SameLine();
		ImGui::TextColored(connectedColor, "%s (0x%02X)", connectedStr, player.ConnectedFlag);

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
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Weapon");
		ImGui::Separator();
		ImGui::Indent(5.0f);

		drawHandle("Primary Weapon:", player.PrimaryWeaponHandle);
		drawHandle("Secondary Weapon:", player.SecondaryWeaponHandle);
		drawHandle("Objective:", player.ObjectiveHandle);

		ImGui::Spacing();
		ImGui::Text("Position: %.3f, %.3f, %.3f",
			player.WeaponPosition[0], player.WeaponPosition[1], player.WeaponPosition[2]);

		ImGui::Text("Forward:  %.3f, %.3f, %.3f",
			player.WeaponForward[0], player.WeaponForward[1], player.WeaponForward[2]);

		ImGui::Text("Aim Offset: %.3f, %.3f, %.3f",
			player.AimOffset[0], player.AimOffset[1], player.AimOffset[2]);

		ImGui::Unindent(5.0f);
		ImGui::Spacing();

		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), "Biped");
		ImGui::Separator();
		ImGui::Indent(5.0f);

		drawHandle("Current Biped:", player.CurrentBipedHandle);
		drawHandle("Alive Biped:", player.AliveBipedHandle);
		drawHandle("Dead Biped:", player.DeadBipedHandle);

		ImGui::Unindent(5.0f);
	}

	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopID();
}