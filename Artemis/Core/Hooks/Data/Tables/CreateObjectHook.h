#pragma once

#include <atomic>

class CreateObjectHook
{
public:
	void Install();
	void Uninstall();

private:
	static unsigned long long __fastcall HookedCreateObject(
		unsigned short* placementData);

	typedef unsigned long long(__fastcall* CreateObject_t)(
		unsigned short* placementData);

	static inline CreateObject_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};