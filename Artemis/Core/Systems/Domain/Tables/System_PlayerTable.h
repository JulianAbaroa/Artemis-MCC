#pragma once

#include "Core/Types/Domain/Tables/PlayerTypes.h"
#include <cstdint>

class System_PlayerTable
{
public:
	void OnPlayerCreated(uint32_t handle);
	void OnPlayerDestroyed(uint32_t handle);

	void FindPlayerTableBase();
	void UpdatePlayerTable();

	void Cleanup();

private:
	LivePlayer BuildLivePlayer(uint32_t handle, const PlayerHeader* header);
	void UpdatePlayerData();
	std::string WideToUtf8(const wchar_t* src, size_t maxLen);
};