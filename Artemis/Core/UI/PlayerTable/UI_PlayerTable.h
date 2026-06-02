#pragma once

#include "Core/Types/Player/LivePlayer.h"

#include "Core/UI/Launcher/UI_Tab.h"

#include "Core/UI/Utils/SearchFilter/PlayerSearchFilter.h"
#include "Core/UI/Utils/Field/CopyableField.h"

#include <unordered_map>

class State_PlayerTable;

class UI_PlayerTable : public UI_Tab
{
public:
    UI_PlayerTable(State_PlayerTable& statePlayerTable) :
        State_PlayerTable(statePlayerTable) {}
    ~UI_PlayerTable() = default;

	void Draw();
	void Cleanup();

private:
    State_PlayerTable& State_PlayerTable;

    std::unordered_map<uint32_t, LivePlayer> m_CachePlayers;
    PlayerSearchFilter m_SearchFilter;
    CopyableField m_CopyableField;

    void DrawPlayerCard(const LivePlayer& player);

    void DrawCardHeader(const LivePlayer& player);
    void DrawConnectionStatus(const LivePlayer& player);
    void DrawCardIdentity(const LivePlayer& player);
    void DrawCardWeapon(const LivePlayer& player);
    void DrawCardBiped(const LivePlayer& player);
};