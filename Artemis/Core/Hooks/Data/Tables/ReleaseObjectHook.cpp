#include "pch.h"
#include "Core/Hooks/Data/Tables/ReleaseObjectHook.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Tables/ObjectTableSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "External/minhook/include/MinHook.h"

void ReleaseObjectHook::HookedReleaseObject(unsigned int handle)
{
	m_OriginalFunction(handle);
	g_pSystem->Domain->ObjectTable->OnObjectDestroyed(handle);
}

void ReleaseObjectHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = 
		(void*)g_pSystem->Infrastructure->Scanner->FindPattern(
			Signatures::ReleaseObject);

	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[ReleaseObject] ERROR:"
			" Failed to obtain the function address.");

		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), 
			&this->HookedReleaseObject, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
	{
		g_pSystem->Debug->Log("[ReleaseObject] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log("[ReleaseObject] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[ReleaseObject] INFO: Hook installed.");
	return;
}

void ReleaseObjectHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[ReleaseObject] INFO: Hook uninstalled.");
}