#pragma once

#include "Core/Types/Domain/Tables/PlayerTypes.h"
#include <unordered_map>

class UI_PlayerTable
{
public:
	void Draw();

	void Cleanup();

private:
	std::unordered_map<uint32_t, LivePlayer> m_CachePlayers;

	void DrawPlayerCard(const LivePlayer& player);
};