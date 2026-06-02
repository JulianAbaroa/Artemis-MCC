#pragma once

#include <atomic>

class Hook_BlamOpenMap;
class Hook_ObjectTable;
class Hook_CreateObject;
class Hook_ReleaseObject;
class Hook_PlayerTable;
class Hook_CreatePlayer;
class Hook_InteractionTable;
class Hook_GetButtonState;
class State_Lifecycle;
class System_AOBScanner;
class System_Logs;

struct Hook_EngineInitialize_Dependencies
{
	Hook_BlamOpenMap& Hook_BlamOpenMap;
	Hook_ObjectTable& Hook_ObjectTable;
	Hook_CreateObject& Hook_CreateObject;
	Hook_ReleaseObject& Hook_ReleaseObject;
	Hook_PlayerTable& Hook_PlayerTable;
	Hook_CreatePlayer& Hook_CreatePlayer;
	Hook_InteractionTable& Hook_InteractionTable;
	Hook_GetButtonState& Hook_GetButtonState;
	State_Lifecycle& State_Lifecycle;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_EngineInitialize
{
public:
	Hook_EngineInitialize(Hook_EngineInitialize_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_EngineInitialize() = default;

	bool Install();
	void Uninstall();

	void* GetFunctionAddress();

private:
	static Hook_EngineInitialize* s_Instance;
	Hook_EngineInitialize_Dependencies m_Deps;

	static void __fastcall HookedEngineInitialize(void);
	
	typedef void(__fastcall* EngineInitialize_t)(void);

	static inline EngineInitialize_t m_OriginalFunction = nullptr;
	std::atomic<bool> m_IsHookInstalled{ false };
	std::atomic<void*> m_FunctionAddress{ nullptr };
};