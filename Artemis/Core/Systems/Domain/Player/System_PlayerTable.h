#pragma once

// Types.
#include "Core/Types/Domain/Player/LivePlayer.h"

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
	LivePlayer BuildLivePlayer(uint32_t handle, uintptr_t playerBase);
	void UpdatePlayerData();
	std::string WideToUtf8(const wchar_t* src, size_t maxLen);
};