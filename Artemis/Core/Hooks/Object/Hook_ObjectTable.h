#pragma once

#include <cstdint>

class State_ObjectTable;
class System_AOBScanner;
class System_Logs;

struct Hook_ObjectTable_Dependencies
{
	State_ObjectTable& State_ObjectTable;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_ObjectTable
{
public:
	Hook_ObjectTable(Hook_ObjectTable_Dependencies dependencies) :
		m_Deps(dependencies) {};
	~Hook_ObjectTable() = default;

	void FindAndStoreTableBase();
	uintptr_t GetObjectTable();

private:
	Hook_ObjectTable_Dependencies m_Deps;
};