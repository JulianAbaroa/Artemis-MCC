#pragma once

class State_InteractionTable;
class System_MemoryReader;
class System_Debug;

struct System_InteractionTable_Dependencies
{
	State_InteractionTable& State_InteractionTable;
	System_MemoryReader& System_MemoryReader;
	System_Debug& System_Debug;
};

class System_InteractionTable
{
public:
	System_InteractionTable(System_InteractionTable_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~System_InteractionTable() = default;

	void UpdateInteractionTable();

	void Cleanup();

private:
	System_InteractionTable_Dependencies m_Deps;
};