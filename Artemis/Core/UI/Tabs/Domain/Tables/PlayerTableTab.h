#pragma once

#include "Core/Common/Types/Domain/Tables/PlayerTypes.h"
#include <unordered_map>

class PlayerTableTab
{
public:
	void Draw();

	void Cleanup();

private:
	std::unordered_map<uint32_t, LivePlayer> m_CachePlayers;

	void DrawPlayerCard(const LivePlayer& player);
};