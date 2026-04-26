#pragma once

#include <atomic>

class ReleaseObjectHook
{
public:
	void Install();
	void Uninstall();

private:
	static void __fastcall HookedReleaseObject(
		unsigned int handle);

	typedef void(__fastcall* ReleaseObject_t)(
		unsigned int handle);

	static inline ReleaseObject_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};