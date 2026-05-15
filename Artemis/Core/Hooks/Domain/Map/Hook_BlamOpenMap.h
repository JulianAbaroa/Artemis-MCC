#pragma once

#include <atomic>

class System_Map;
class System_MapTagGroup;
class System_Navigation;
class System_Environment;
class System_Interactable;
class System_AOBScanner;
class System_Debug;

struct Hook_BlamOpenMap_Dependencies
{
	System_Map& System_Map;
	System_MapTagGroup& System_MapTagGroup;
	System_Navigation& System_Navigation;
	System_Environment& System_Environment;
	System_Interactable& System_Interactable;
	System_AOBScanner& System_AOBScanner;
	System_Debug& System_Debug;
};

class Hook_BlamOpenMap
{
public:
	Hook_BlamOpenMap(Hook_BlamOpenMap_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_BlamOpenMap() = default;

	void Install();
	void Uninstall();

private:
	static Hook_BlamOpenMap* s_Instance;
	Hook_BlamOpenMap_Dependencies m_Deps;

	static void __fastcall HookedBlamOpenMap(
		uint64_t param_1, uint64_t param_2,
		uint64_t mapRelativePath, uint32_t* param_4);

	typedef void(__fastcall* BlamOpenMap_t)(
		uint64_t param_1, uint64_t param_2, 
		uint64_t mapRelativePath, uint32_t* param_4);

	static inline BlamOpenMap_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};