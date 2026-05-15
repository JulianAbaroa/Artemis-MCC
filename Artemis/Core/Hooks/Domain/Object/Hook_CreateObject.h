#pragma once

#include <atomic>

class System_ObjectTable;
class System_AOBScanner;
class System_Debug;

struct Hook_CreateObject_Dependencies
{
	System_ObjectTable& System_ObjectTable;
	System_AOBScanner& System_AOBScanner;
	System_Debug& System_Debug;
};

class Hook_CreateObject
{
public:
	Hook_CreateObject(Hook_CreateObject_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_CreateObject() = default;

	void Install();
	void Uninstall();

private:
	static Hook_CreateObject* s_Instance;
	Hook_CreateObject_Dependencies m_Deps;

	static unsigned long long __fastcall HookedCreateObject(
		unsigned short* placementData);

	typedef unsigned long long(__fastcall* CreateObject_t)(
		unsigned short* placementData);

	static inline CreateObject_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};