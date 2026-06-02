#pragma once

#include <cstdint>
#include <atomic>

class System_AOBScanner;
class System_Logs;

struct Hook_GameEngineInit_Dependencies
{
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_GameEngineInit
{
public:
	Hook_GameEngineInit(Hook_GameEngineInit_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_GameEngineInit() = default;

	bool Install();
	void Uninstall();

	void* GetFunctionAddress();

private:
	static Hook_GameEngineInit* s_Instance;
	Hook_GameEngineInit_Dependencies m_Deps;

	static void __fastcall HookedGameEngineInit(
		uint64_t param_1, uint64_t pSystem, uint64_t* pConfiguration);
	
	typedef void(__fastcall* GameEngineStart_t)(uint64_t, uint64_t, uint64_t*);

	static inline GameEngineStart_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};