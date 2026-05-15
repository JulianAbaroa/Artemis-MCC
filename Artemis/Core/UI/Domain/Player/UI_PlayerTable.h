#pragma once

// Types.
#include "Core/Types/Domain/Player/LivePlayer.h"

// Utils.
#include "Core/UI/Utils/PlayerSearchFilter.h"
#include "Core/UI/Utils/CopyableField.h"

#include <unordered_map>

class State_PlayerTable;
class System_Debug;

struct UI_PlayerTable_Dependencies
{
    State_PlayerTable& State_PlayerTable;
    System_Debug& System_Debug;
};

class UI_PlayerTable
{
public:
    UI_PlayerTable(UI_PlayerTable_Dependencies dependencies) :
        m_Deps(dependencies) {}
    ~UI_PlayerTable() = default;

	void Draw();
	void Cleanup();

private:
    UI_PlayerTable_Dependencies m_Deps;

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