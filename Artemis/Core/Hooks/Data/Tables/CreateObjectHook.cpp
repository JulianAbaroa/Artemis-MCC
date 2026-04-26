#include "pch.h"
#include "Core/Hooks/Data/Tables/CreateObjectHook.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Tables/ObjectTableSystem.h"
#include "Core/Systems/Infrastructure/CoreInfrastructureSystem.h"
#include "Core/Systems/Infrastructure/Engine/ScannerSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"
#include "External/minhook/include/MinHook.h"

unsigned long long __fastcall CreateObjectHook::HookedCreateObject(unsigned short* placementData)
{
	auto handle = m_OriginalFunction(placementData);
	uint32_t datumIndex = *(uint32_t*)((unsigned char*)placementData + 0x00);

	g_pSystem->Domain->ObjectTable->OnObjectCreated(
		static_cast<uint32_t>(handle), datumIndex);

	return handle;
}

void CreateObjectHook::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = 
		(void*)g_pSystem->Infrastructure->Scanner->FindPattern(
			Signatures::CreateObject);

	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[CreateObject] ERROR:"
			" Failed to obtain the function address.");

		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), 
			&this->HookedCreateObject, 
			reinterpret_cast<LPVOID*>(&m_OriginalFunction)
		) != MH_OK)
	{
		g_pSystem->Debug->Log("[CreateObject] ERROR:"
			" Failed to create the hook.");

		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log(" [CreateObject] ERROR:"
			" Failed to enable hook.");

		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[CreateObject] INFO: Hook installed.");
	return;
}

void CreateObjectHook::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[CreateObject] INFO: Hook uninstalled.");
}