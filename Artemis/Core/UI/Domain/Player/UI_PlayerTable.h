#pragma once

// Types.
#include "Core/Types/Domain/Player/LivePlayer.h"

// Utils.
#include "Core/UI/Utils/PlayerSearchFilter.h"
#include "Core/UI/Utils/CopyableField.h"

#include <unordered_map>

class UI_PlayerTable
{
public:
	void Draw();
	void Cleanup();

private:
    std::unordered_map<uint32_t, LivePlayer> m_CachePlayers;
    PlayerSearchFilter m_SearchFilter;
    CopyableField m_CopyableField;

    void DrawPlayerCard(const LivePlayer& player);

    void DrawCardHeader(const LivePlayer& player);
    void DrawCardIdentity(const LivePlayer& player);
    void DrawCardWeapon(const LivePlayer& player);
    void DrawCardBiped(const LivePlayer& player);

    void DrawConnectionStatus(const LivePlayer& player);
    void DrawAliveStatus(const LivePlayer& player);
};