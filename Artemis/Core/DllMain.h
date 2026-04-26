#pragma once

#include <shlwapi.h>
#include <thread>

class ModLoader
{
public:
	BOOL OnAttach(HMODULE hModule);
	void OnDetach(LPVOID lpReserved);

private:
	static DWORD WINAPI InitializeArtemis(LPVOID lpParam);
	void DeinitializeArtemis(LPVOID lpReserved);

	std::thread m_MainThread{};
	std::thread m_InputThread{};
	
	std::thread m_AIThread{};
};