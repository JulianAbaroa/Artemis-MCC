#pragma once

#include <atomic>

class BlamOpenMapHook
{
public:
	void Install();
	void Uninstall();

private:
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