#pragma once

#include <cstdint>
#include <atomic>

class Hook_GameEngineInit
{
public:
	bool Install(bool silent);
	void Uninstall();

	void* GetFunctionAddress();

private:
	static void __fastcall HookedGameEngineInit(
		uint64_t param_1, uint64_t pSystem, uint64_t* pConfiguration);
	
	typedef void(__fastcall* GameEngineStart_t)(uint64_t, uint64_t, uint64_t*);

	static inline GameEngineStart_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};