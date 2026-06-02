#pragma once

#include <atomic>

class Hook_BlamOpenMap;
class Hook_CreateObject;
class Hook_ReleaseObject;
class Hook_CreatePlayer;
class Hook_GetButtonState;
class State_Lifecycle;
class System_MapReader;
class System_TagGroupReader;
class System_ObjectTable;
class System_PlayerTable;
class System_InteractionTable;
class System_Classifier;
class System_ObjectGraph;
class System_PlayerGraph;
class System_Navigation;
class System_Environment;
class System_Interactable;
class System_AOBScanner;
class System_Logs;
class UI_ObjectTable;
class UI_PlayerTable;
class UI_Navigation;
class UI_Interactable;
class UI_ObjectTable;
class UI_PlayerTable;
class UI_ObjectGraph;
class UI_Map;

struct Hook_DestroySubsystems_Dependencies
{
	Hook_BlamOpenMap& Hook_BlamOpenMap;
	Hook_CreateObject& Hook_CreateObject;
	Hook_ReleaseObject& Hook_ReleaseObject;
	Hook_CreatePlayer& Hook_CreatePlayer;
	Hook_GetButtonState& Hook_GetButtonState;
	State_Lifecycle& State_Lifecycle;
	System_MapReader& System_MapReader;
	System_TagGroupReader& System_TagGroup;
	System_ObjectTable& System_ObjectTable;
	System_PlayerTable& System_PlayerTable;
	System_InteractionTable& System_InteractionTable;
	System_Classifier& System_Classifier;
	System_ObjectGraph& System_ObjectGraph;
	System_PlayerGraph& System_PlayerGraph;
	System_Navigation& System_Navigation;
	System_Environment& System_Environment;
	System_Interactable& System_Interactable;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
	UI_ObjectTable& UI_ObjectTable;
	UI_PlayerTable& UI_PlayerTable;
	UI_Map& UI_Map;
};

class Hook_DestroySubsystems
{
public:
	Hook_DestroySubsystems(Hook_DestroySubsystems_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_DestroySubsystems() = default;

	bool Install();
	void Uninstall();

	void* GetFunctionAddress();

private:
	static Hook_DestroySubsystems* s_Instance;
	Hook_DestroySubsystems_Dependencies m_Deps;

	static void __fastcall HookedDestroySubsystems(void);

	typedef void(__fastcall* DestroySubsystems_t)(void);

	static inline DestroySubsystems_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};