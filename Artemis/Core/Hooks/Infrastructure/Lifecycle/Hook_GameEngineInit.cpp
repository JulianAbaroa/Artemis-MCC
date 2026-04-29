#include "pch.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_GameEngineInit.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"

void __fastcall Hook_GameEngineInit::HookedGameEngineInit(
	uint64_t param_1, uint64_t pSystem, uint64_t* pConfiguration)
{
	m_OriginalFunction(param_1, pSystem, pConfiguration);

	// TODO: Can we get the gametype here?
}

bool Hook_GameEngineInit::Install(bool silent)
{
	if (m_IsHookInstalled.load()) return true;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::GameEngineStart);
	if (!functionAddress)
	{
		if (!silent)  g_pSystem->Debug->Log("[GameEngineStart] ERROR: Failed to obtain the function address.");
		return false;
	}

	m_FunctionAddress.store(functionAddress);
	MH_RemoveHook(m_FunctionAddress.load());
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedGameEngineInit, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[GameEngineStart] ERROR: Failed to create the hook.");
		return false;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK) 
	{
		g_pSystem->Debug->Log("[GameEngineStart] ERROR: Failed to enable the hook.");
		return false;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[GameEngineStart] INFO: Hook installed.");
	return true;
}

void Hook_GameEngineInit::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[GameEngineStart] INFO: Hook uninstalled.");
}

void* Hook_GameEngineInit::GetFunctionAddress()
{
	return m_FunctionAddress.load();
}