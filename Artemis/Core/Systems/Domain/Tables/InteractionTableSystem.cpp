#include "pch.h"
#include "Core/Hooks/CoreHook.h"
#include "Core/Hooks/Memory/CoreMemoryHook.h"
#include "Core/Hooks/Memory/InteractionTableHook.h"
#include "Core/States/CoreState.h"
#include "Core/States/Domain/CoreDomainState.h"
#include "Core/States/Domain/Tables/InteractionTableState.h"
#include "Core/Systems/CoreSystem.h"
#include "Core/Systems/Domain/Tables/InteractionTableSystem.h"
#include "Core/Systems/Interface/DebugSystem.h"

void InteractionTableSystem::FindInteractionTableBase()
{
	uintptr_t tableBase = g_pState->Domain->InteractionTable->GetInteractionTableBase();
	if (tableBase == 0)
	{
		tableBase = g_pHook->Memory->InteractionTable->GetInteractionTable();
		if (!tableBase)
		{
			g_pSystem->Debug->Log("[InteractionTableSystem] ERROR:"
				" InteractionTableBase invalid.");

			return;
		}

		g_pSystem->Debug->Log("[InteractionTableSystem] INFO: InteractionTable: 0x%llX", tableBase);
		g_pState->Domain->InteractionTable->SetInteractionTableBase(tableBase);
	}
}

void InteractionTableSystem::UpdateInteractionTable()
{
	uintptr_t base = g_pState->Domain->InteractionTable->GetInteractionTableBase();
	if (!base)
	{
		g_pState->Domain->InteractionTable->SetLiveInteraction({});
		return;
	}

	__try
	{
		const InteractionTable* raw = 
			reinterpret_cast<const InteractionTable*>(base);

		LiveInteraction live;
		
		// Object.
		live.Type = (InteractionType)raw->InteractionType;
		live.InteractionSlotID = raw->InteractionSlotID;
		live.TargetObjectHandle = raw->TargetObjectHandle;

		// Player: Melee.
		live.IsMeleeAvailable = raw->IsMeleeAvailable;
		live.MeleeTargetHandle = raw->MeleeTargetHandle;

		// Player: Aim.
		live.IsAimAvailable = raw->IsAimAvailable;
		live.BodyPart = (BodyPart)raw->BodyPart;
		live.AimTargetHandle = raw->AimTargetHandle;
		live.AimTargetSlotID = raw->AimTargetSlotID;
		live.AimHitLocalPosition[0] = raw->AimHitLocalPosition[0];
		live.AimHitLocalPosition[1] = raw->AimHitLocalPosition[1];
		live.AimHitLocalPosition[2] = raw->AimHitLocalPosition[2];

		g_pState->Domain->InteractionTable->SetLiveInteraction(live);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{  
		g_pState->Domain->InteractionTable->SetLiveInteraction({});
	}
}

void InteractionTableSystem::Cleanup()
{
	g_pState->Domain->InteractionTable->Cleanup();
	g_pSystem->Debug->Log("[InteractionTableSystem] INFO: Cleanup completed.");
}