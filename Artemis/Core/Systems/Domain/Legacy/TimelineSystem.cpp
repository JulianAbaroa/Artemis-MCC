#include "pch.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Legacy/TimelineState.h"
#include "Core/States/Domain/Legacy/EventRegistryState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/CoreDomainSystem.h"
#include "Core/Systems/Domain/Legacy/TimelineSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

//void TimelineSystem::ProcessEngineEvent(std::wstring& templateStr, EventData* rawData)
//{
//	EventType type = g_pState->Domain->EventRegistry->GetEventType(templateStr);
//	std::vector<PlayerInfo> players = ExtractPlayers(rawData);
//
//	EventTeams teams = { rawData->CauseTeam, rawData->EffectTeam };
//
//	GameEvent event { 
//		type, 
//		{ rawData->CauseTeam, rawData->EffectTeam }, 
//		players 
//	};
//
//	g_pState->Domain->Timeline->AddGameEvent(event);
//}

//std::vector<PlayerInfo> TimelineSystem::ExtractPlayers(EventData* rawData)
//{
//	std::vector<PlayerInfo> detectedPlayers;
//	if (!rawData) return detectedPlayers;
//
//	auto AddPlayerBySlot = [&](uint8_t slotIndex) {
//		auto player = g_pState->Domain->Theater->GetPlayerBySlot(slotIndex);
//
//		if (player && player->RawPlayer.Name[0] != L'\0')
//		{
//			detectedPlayers.push_back(*player);
//			return true;
//		}
//
//		return false;
//	};
//
//	AddPlayerBySlot(rawData->CauseSlotIndex);
//
//	if (rawData->EffectSlotIndex != rawData->CauseSlotIndex)
//	{
//		AddPlayerBySlot(rawData->EffectSlotIndex);
//	}
//
//	return detectedPlayers;
//}

void TimelineSystem::Cleanup()
{
	g_pSystem->Debug->Log("[TimelineSystem] INFO: Cleanup completed.");
}