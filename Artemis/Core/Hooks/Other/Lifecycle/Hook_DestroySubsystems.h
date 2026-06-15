#pragma once

#include <atomic>

class Hook_BlamOpenMap;
class Hook_CreateObject;
class Hook_ReleaseObject;
class Hook_InitRootNode;
class Hook_CreatePlayer;
class Hook_SimulationTicks;
class Hook_GetButtonState;
class State_Lifecycle;
class System_MapReader;
class System_TagGroupReader;
class System_GeometryReader;
class System_ObjectTable;
class System_PlayerTable;
class System_InteractionTable;
class System_Classifier;
class System_ObjectGraph;
class System_PlayerGraph;
class System_WorldBuilder;
class System_Collidables;
class System_StatsBuilder;
class System_VitalityBuilder;
class System_Vitality;
class System_Self;
class System_Fixtures;
class System_Affordances;
class System_AOBScanner;
class System_Logs;
class UI_ObjectTable;
class UI_PlayerTable;
class UI_Navigation;
class UI_Interactable;
class UI_ObjectTable;
class UI_PlayerTable;
class UI_ObjectGraph;
class System_MapRenderer;

struct Hook_DestroySubsystems_Dependencies
{
	Hook_BlamOpenMap& Hook_BlamOpenMap;
	Hook_CreateObject& Hook_CreateObject;
	Hook_ReleaseObject& Hook_ReleaseObject;
	Hook_InitRootNode& Hook_InitRootNode;
	Hook_CreatePlayer& Hook_CreatePlayer;
	Hook_SimulationTicks& Hook_SimulationTicks;
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
	System_WorldBuilder& System_WorldBuilder;
	System_Collidables& System_Collidables;
	System_StatsBuilder& System_StatsBuilder;
	System_VitalityBuilder& System_VitalityBuilder;
	System_Vitality& System_Vitality;
	System_Self& System_Self;
	System_Fixtures& System_Fixtures;
	System_Affordances& System_Affordances;
	System_MapRenderer& System_MapRenderer;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
	UI_ObjectTable& UI_ObjectTable;
	UI_PlayerTable& UI_PlayerTable;
};

class Hook_DestroySubsystems
{
public:
	Hook_DestroySubsystems(Hook_DestroySubsystems_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_DestroySubsystems() = default;

	static inline std::atomic<bool> s_InProgress{ false };

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