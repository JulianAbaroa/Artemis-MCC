#pragma once

#include <cstdint>

class State_PlayerTable;
class System_AOBScanner;
class System_Logs;

struct Hook_PlayerTable_Dependencies
{
	State_PlayerTable& State_PlayerTable;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_PlayerTable
{
public:
	Hook_PlayerTable(Hook_PlayerTable_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_PlayerTable() = default;

	void FindAndStoreTableBase();
	uintptr_t GetPlayerTable();

private:
	Hook_PlayerTable_Dependencies m_Deps;
};