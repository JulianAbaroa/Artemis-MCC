#pragma once

#include <atomic>

class System_PlayerTable;
class System_AOBScanner;
class System_Logs;

struct Hook_CreatePlayer_Dependencies
{
	System_PlayerTable& System_PlayerTable;
	System_AOBScanner& System_AOBScanner;
	System_Logs& System_Logs;
};

class Hook_CreatePlayer
{
public:
	Hook_CreatePlayer(Hook_CreatePlayer_Dependencies dependencies) :
		m_Deps(dependencies) {}
	~Hook_CreatePlayer() = default;

	void Install();
	void Uninstall();

private:
	static Hook_CreatePlayer* s_Instance;
	Hook_CreatePlayer_Dependencies m_Deps;

	static uint32_t __fastcall HookedCreatePlayer(
		uint32_t playerIndex, uint64_t pPlayerInfo, uint8_t playerFlags);

	typedef uint32_t(__fastcall* CreatePlayer_t)(
		uint32_t playerIndex, uint64_t pPlayerInfo, uint8_t playerFlags);

	static inline CreatePlayer_t m_OriginalFunction = nullptr;
	std::atomic<void*> m_FunctionAddress{ nullptr };
	std::atomic<bool> m_IsHookInstalled{ false };
};