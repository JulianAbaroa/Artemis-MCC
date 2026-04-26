#pragma once

#include <atomic>

class CreatePlayerHook
{
public:
	void Install();
	void Uninstall();

private:
	static uint32_t __fastcall HookedCreatePlayer(
		uint32_t playerIndex, uint64_t pPlayerInfo, uint8_t playerFlags);

	typedef uint32_t(__fastcall* CreatePlayer_t)(
		uint32_t playerIndex, uint64_t pPlayerInfo, uint8_t playerFlags);

	static inline CreatePlayer_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};