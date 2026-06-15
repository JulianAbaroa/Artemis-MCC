#pragma once

#include <cstdint>
#include <string>

struct LivePlayer;

class State_PlayerTable;
class System_MemoryReader;
class System_Logs;

struct Sys_PlayerTable_Deps
{
	State_PlayerTable& State_PlayerTable;
	System_MemoryReader& System_MemoryReader;
	System_Logs& System_Logs;
};

class System_PlayerTable
{
public:
	System_PlayerTable(Sys_PlayerTable_Deps deps) : m_Deps(deps) {}
	~System_PlayerTable() = default;

	void OnPlayerCreated(uint32_t handle);

	void UpdatePlayerTable();

	void Cleanup();

private:
	Sys_PlayerTable_Deps m_Deps;

	LivePlayer BuildLivePlayer(uint32_t handle, uintptr_t playerBase);
	
	void UpdatePlayerData();

	// --- Helpers ---
	std::string WideToUtf8(const wchar_t* source, size_t maxLength);
};