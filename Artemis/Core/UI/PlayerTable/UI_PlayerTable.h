#pragma once

#include "Core/Types/Sources/Tables/Player/LivePlayer.h"

#include "Core/states/Sources/Tables/Player/State_PlayerTable.h"

#include "Core/UI/Launcher/UI_Tab.h"

#include "Core/UI/Utils/SearchFilter/PlayerSearchFilter.h"
#include "Core/UI/Utils/Field/CopyableField.h"

struct Tick;

class UI_PlayerTable : public UI_Tab
{
public:
    UI_PlayerTable(State_PlayerTable& statePlayerTable) :
        State_PlayerTable(statePlayerTable) {}
    ~UI_PlayerTable() = default;

	void Draw(std::shared_ptr<const Tick> tick);

	void Cleanup();

private:
    State_PlayerTable& State_PlayerTable;

    std::shared_ptr<const PlayerTable> m_PlayerTable;
    PlayerSearchFilter m_SearchFilter;
    CopyableField m_CopyableField;

    void DrawPlayerCard(const LivePlayer& player);

    void DrawCardHeader(const LivePlayer& player);
    void DrawConnectionStatus(const LivePlayer& player);
    void DrawCardIdentity(const LivePlayer& player);
    void DrawCardWeapon(const LivePlayer& player);
    void DrawCardBiped(const LivePlayer& player);
};