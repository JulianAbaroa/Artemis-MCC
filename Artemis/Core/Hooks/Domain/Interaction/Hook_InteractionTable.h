#pragma once

class State_InteractionTable;
class System_AOBScanner;
class System_Debug;

struct Hook_InteractionTable_Dependencies
{
	State_InteractionTable& State_InteractionTable;
	System_AOBScanner& System_AOBScanner;
	System_Debug& System_Debug;
};

class Hook_InteractionTable
{
public:
	Hook_InteractionTable(Hook_InteractionTable_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_InteractionTable() = default;

	void FindAndStoreTableBase();
	uintptr_t GetInteractionTable();

private:
	Hook_InteractionTable_Dependencies m_Deps;
};