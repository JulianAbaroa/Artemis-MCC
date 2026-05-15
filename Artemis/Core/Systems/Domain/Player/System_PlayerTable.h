#pragma once

// Types.
#include "Core/Types/Domain/Player/LivePlayer.h"

#include <cstdint>

class State_PlayerTable;
class System_MemoryReader;
class System_Debug;

struct System_PlayerTable_Dependencies
{
	State_PlayerTable& State_PlayerTable;
	System_MemoryReader& System_MemoryReader;
	System_Debug& System_Debug;
};

class System_PlayerTable
{
public:
	System_PlayerTable(System_PlayerTable_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~System_PlayerTable() = default;

	void OnPlayerCreated(uint32_t handle);

	void UpdatePlayerTable();

	void Cleanup();

private:
	System_PlayerTable_Dependencies m_Deps;

	LivePlayer BuildLivePlayer(uint32_t handle, uintptr_t playerBase);
	void UpdatePlayerData();
	std::string WideToUtf8(const wchar_t* src, size_t maxLen);
};