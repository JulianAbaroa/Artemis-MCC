#pragma once

class State_InteractionTable;
class System_MemoryReader;
class System_Logs;

struct Sys_InteractionTable_Deps
{
	State_InteractionTable& State_InteractionTable;
	System_MemoryReader& System_MemoryReader;
	System_Logs& System_Logs;
};

class System_InteractionTable
{
public:
	System_InteractionTable(Sys_InteractionTable_Deps deps) : m_Deps(deps) {}
	~System_InteractionTable() = default;

	void UpdateInteractionTable();

	void Cleanup();

private:
	Sys_InteractionTable_Deps m_Deps;
};