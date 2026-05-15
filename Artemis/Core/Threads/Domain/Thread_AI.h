#pragma once

class State_Map;
class State_PlayerTable;
class State_Navigation;
class State_Lifecycle;
class System_ObjectTable;
class System_PlayerTable;
class System_InteractionTable;
class System_ObjectClassifier;
class System_ObjectGraph;
class System_PlayerGraph;
class System_Navigation;
class System_Environment;
class System_Interactable;
class System_Debug;

struct Thread_AI_Dependencies
{
	State_Map& State_Map;
	State_PlayerTable& State_PlayerTable;
	State_Navigation& State_Navigation;
	State_Lifecycle& State_Lifecycle;
	System_ObjectTable& System_ObjectTable;
	System_PlayerTable& System_PlayerTable;
	System_InteractionTable& System_InteractionTable;
	System_ObjectClassifier& System_ObjectClassifier;
	System_ObjectGraph& System_ObjectGraph;
	System_PlayerGraph& System_PlayerGraph;
	System_Navigation& System_Navigation;
	System_Environment& System_Environment;
	System_Interactable& System_Interactable;
	System_Debug& System_Debug;
};

class Thread_AI
{
public:
	Thread_AI(Thread_AI_Dependencies dependencies) : 
		m_Deps(dependencies) {}
	~Thread_AI() = default;

	void Run();

private:
	Thread_AI_Dependencies m_Deps;
};