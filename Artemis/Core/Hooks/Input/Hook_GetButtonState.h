#pragma once

#include <atomic>

class State_Input;
class System_AOBScanner;
class System_Logs;

struct Hook_GetButtonState_Dependencies
{
	State_Input& State_Input;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_GetButtonState
{
public:
	Hook_GetButtonState(Hook_GetButtonState_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_GetButtonState() = default;

	void Install();
	void Uninstall();

private:
	static Hook_GetButtonState* s_Instance;
	Hook_GetButtonState_Dependencies m_Deps;

	static char __fastcall HookedGetButtonState(short buttonID);

	typedef char(__fastcall* GetButtonState_t)(short buttonID);

	static inline GetButtonState_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};
