#include "pch.h"

// Header
#include "System_InteractionTable.h"

// Types.
#include "Core/Types/Domain/Interaction/InteractionOffsets.h"

// Hooks.
#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"
#include "Core/Hooks/Domain/Interaction/Hook_InteractionTable.h"

// States.
#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Domain/Interaction/State_InteractionTable.h"

// Systems.
#include "Core/Systems/Core_System.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"

#include "Core/Systems/Interface/System_Debug.h"

#include "Core/Systems/Infrastructure/Engine/Memory/System_MemoryReader.h"

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
	uintptr_t tableBase = g_pState->Domain->InteractionTable->GetInteractionTableBase();
	if (!tableBase)
	{
		g_pState->Domain->InteractionTable->SetLiveInteraction({});
		return;
	}

	__try
	{
		namespace Interaction = InteractionOffsets;

		LiveInteraction live;

		// Object.

		auto rawType = g_pSystem->Infrastructure->MemoryReader->Read<uint8_t>(
			tableBase, Interaction::InteractionType);
		live.Type = static_cast<InteractionType>(rawType);

		auto rawDetail = g_pSystem->Infrastructure->MemoryReader->Read<uint8_t>(
			tableBase, Interaction::InteractionDetail);
		live.InteractionSlotID = static_cast<InteractionDetail>(rawDetail);

		live.TargetObjectHandle = g_pSystem->Infrastructure->MemoryReader->Read<uint32_t>(
			tableBase, Interaction::TargetObjectHandle);

		// Player: Melee.
		live.IsMeleeAvailable = g_pSystem->Infrastructure->MemoryReader->Read<uint8_t>(
			tableBase, Interaction::IsMeleeAvailable);

		live.MeleeTargetHandle = g_pSystem->Infrastructure->MemoryReader->Read<uint32_t>(
			tableBase, Interaction::MeleeTargetHandle);

		// Player: Aim.
		live.IsAimAvailable = g_pSystem->Infrastructure->MemoryReader->Read<uint8_t>(
			tableBase, Interaction::IsAimAvailable);

		auto rawBodyPart = g_pSystem->Infrastructure->MemoryReader->Read<uint8_t>(
			tableBase, Interaction::BipedBodyPart);
		live.BodyPart = static_cast<BodyPart>(rawBodyPart);

		live.AimTargetHandle = g_pSystem->Infrastructure->MemoryReader->Read<uint32_t>(
			tableBase, Interaction::AimTargetHandle);

		live.AimTargetSlotID = g_pSystem->Infrastructure->MemoryReader->Read<uint32_t>(
			tableBase, Interaction::AimTargetSlotID);

		live.AimHitLocalPosition = g_pSystem->Infrastructure->MemoryReader->ReadArray<float, 3>(
			tableBase, Interaction::AimHitLocalPosition);

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