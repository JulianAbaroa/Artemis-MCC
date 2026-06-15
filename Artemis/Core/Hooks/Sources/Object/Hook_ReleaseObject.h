#pragma once

#include <atomic>

class System_ObjectTable;
class System_AOBScanner;
class System_Logs;

struct Hook_ReleaseObject_Dependencies
{
	System_ObjectTable& System_ObjectTable;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_ReleaseObject
{
public:
	Hook_ReleaseObject(Hook_ReleaseObject_Dependencies dependencies) :
		m_Deps(dependencies) {};
	~Hook_ReleaseObject() = default;

	void Install();
	void Uninstall();

private:
	static Hook_ReleaseObject* s_Instance;
	Hook_ReleaseObject_Dependencies m_Deps;

	static void __fastcall HookedReleaseObject(
		unsigned int handle);

	typedef void(__fastcall* ReleaseObject_t)(
		unsigned int handle);

	static inline ReleaseObject_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};