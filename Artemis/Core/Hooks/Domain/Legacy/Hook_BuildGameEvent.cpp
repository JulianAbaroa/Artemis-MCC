#include "pch.h"
#include "Core/Hooks/Domain/Legacy/Hook_BuildGameEvent.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Domain/Legacy/System_Timeline.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Infrastructure/Engine/System_Scanner.h"
#include "Core/Systems/Interface/System_Debug.h"
#include "External/minhook/include/MinHook.h"

// This function acts as a string formatter for the in-game event feed (kill-feed).
// It processes a 'pTemplateStr' and uses 'pEventData' to generate the final localized
// string displayed to players.
// Observed Behavior: This hook does not capture every game event. While it processes
// major combat and world events, it does not appear to trigger for minor local 
// actions such as picking up weapons or equipment.
unsigned char Hook_BuildGameEvent::HookedBuildGameEvent(
	int playerMask, wchar_t* pTemplateStr, void* pEventData,
	unsigned int flags, wchar_t* pOutBuffer) 
{
	unsigned char result = m_OriginalFunction(playerMask, 
		pTemplateStr, pEventData, flags, pOutBuffer);
	
	if (!result || !pOutBuffer || pOutBuffer[0] == L'\0' || !pEventData)
	{
		return result;
	}

	EventData* eventData = (EventData*)pEventData;
	std::wstring currentTemplate(pTemplateStr);

	// PrintRawEvent(pOutBuffer);

	//g_pSystem->Domain->Timeline->ProcessEngineEvent(currentTemplate, eventData);

	return result;
}

void Hook_BuildGameEvent::Install()
{
	if (m_IsHookInstalled.load()) return;

	void* functionAddress = (void*)g_pSystem->Infrastructure->Scanner->FindPattern(Signatures::BuildGameEvent);
	if (!functionAddress)
	{
		g_pSystem->Debug->Log("[BuildGameEvent] ERROR: Failed to obtain the function address.");
		return;
	}

	m_FunctionAddress.store(functionAddress);
	if (MH_CreateHook(m_FunctionAddress.load(), &this->HookedBuildGameEvent, reinterpret_cast<LPVOID*>(&m_OriginalFunction)) != MH_OK)
	{
		g_pSystem->Debug->Log("[BuildGameEvent] ERROR: Failed to create the hook.");
		return;
	}
	if (MH_EnableHook(m_FunctionAddress.load()) != MH_OK)
	{
		g_pSystem->Debug->Log("[BuildGameEvent] ERROR: Failed to enable hook.");
		return;
	}

	m_IsHookInstalled.store(true);
	g_pSystem->Debug->Log("[BuildGameEvent] INFO: Hook installed.");
}

void Hook_BuildGameEvent::Uninstall()
{
	if (!m_IsHookInstalled.load()) return;

	MH_DisableHook(m_FunctionAddress.load());
	MH_RemoveHook(m_FunctionAddress.load());

	m_IsHookInstalled.store(false);
	g_pSystem->Debug->Log("[BuildGameEvent] INFO: Hook uninstalled.");
}


//void BuildGameEventHook::PrintNewEvent(wchar_t* pTemplateStr, void* pEventData,
//	wchar_t* pOutBuffer, std::wstring currentTemplate, float currentTime)
//{
//	bool alreadyMapped = g_pState->Domain->EventRegistry->IsEventRegistered(currentTemplate);
//	if (!alreadyMapped) 
//	{
//		EventData* data = (EventData*)pEventData;
//		std::string timeStr = g_pSystem->Infrastructure->Format->ToTimestamp(currentTime);
//
//		g_pSystem->Debug->Log(
//			"[BuildGameEvent] INFO: [%s] [NEW EVENT] Template: %ls | Msg: %ls | Slot: %d | Val: %d",
//			timeStr.c_str(), pTemplateStr, pOutBuffer, data->CauseSlotIndex, data->CustomValue);
//	}
//}

void Hook_BuildGameEvent::PrintRawEvent(wchar_t* pOutBuffer)
{
	g_pSystem->Debug->Log("[BuildGameEvent] INFO: '%ls'", pOutBuffer);
}