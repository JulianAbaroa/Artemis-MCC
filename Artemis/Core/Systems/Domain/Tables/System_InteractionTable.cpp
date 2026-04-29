#include "pch.h"
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"
#include "Core/Hooks/Domain/Tables/Hook_InteractionTable.h"
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Tables/State_InteractionTable.h"
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Tables/System_InteractionTable.h"
#include "Core/Systems/Interface/System_Debug.h"

void System_InteractionTable::FindInteractionTableBase()
{
	uintptr_t tableBase = g_pState->Domain->InteractionTable->GetInteractionTableBase();
	if (tableBase == 0)
	{
		tableBase = g_pHook->Domain->InteractionTable->GetInteractionTable();
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

void System_InteractionTable::UpdateInteractionTable()
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

void System_InteractionTable::Cleanup()
{
	g_pState->Domain->InteractionTable->Cleanup();
	g_pSystem->Debug->Log("[InteractionTableSystem] INFO: Cleanup completed.");
}