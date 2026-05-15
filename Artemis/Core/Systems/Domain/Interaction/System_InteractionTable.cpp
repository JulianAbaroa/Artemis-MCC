#include "pch.h"

// Header
#include "System_InteractionTable.h"

// Types.
#include "Core/Types/Domain/Interaction/InteractionOffsets.h"

// --- States ---

#include "Core/States/Domain/Interaction/State_InteractionTable.h"

// --- Systems ---

#include "Core/Systems/Infrastructure/Engine/Memory/System_MemoryReader.h"

#include "Core/Systems/Interface/Debug/System_Debug.h"

void System_InteractionTable::UpdateInteractionTable()
{
	uintptr_t tableBase = m_Deps.State_InteractionTable.GetInteractionTableBase();
	if (!tableBase)
	{
		m_Deps.State_InteractionTable.SetLiveInteraction({});
		return;
	}

	__try
	{
		namespace Interaction = InteractionOffsets;

		auto& reader = m_Deps.System_MemoryReader;

		LiveInteraction live;

		// Object.
		live.Type = reader.Read<InteractionType>(tableBase, Interaction::InteractionType);
		live.InteractionSlotID = reader.Read<InteractionDetail>(tableBase, Interaction::InteractionDetail);
		live.TargetObjectHandle = reader.Read<uint32_t>(tableBase, Interaction::TargetObjectHandle);

		// Player: Melee.
		live.IsMeleeAvailable = reader.Read<uint8_t>(tableBase, Interaction::IsMeleeAvailable);
		live.MeleeTargetHandle = reader.Read<uint32_t>(tableBase, Interaction::MeleeTargetHandle);

		// Player: Aim.
		live.IsAimAvailable = reader.Read<uint8_t>(tableBase, Interaction::IsAimAvailable);
		live.BodyPart = reader.Read<BodyPart>(tableBase, Interaction::BipedBodyPart);
		live.AimTargetHandle = reader.Read<uint32_t>(tableBase, Interaction::AimTargetHandle);
		live.AimTargetSlotID = reader.Read<uint32_t>(tableBase, Interaction::AimTargetSlotID);
		live.AimHitLocalPosition = reader.ReadArray<float, 3>(tableBase, Interaction::AimHitLocalPosition);

		m_Deps.State_InteractionTable.SetLiveInteraction(live);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) 
	{  
		m_Deps.State_InteractionTable.SetLiveInteraction({});
	}
}

void System_InteractionTable::Cleanup()
{
	m_Deps.State_InteractionTable.Cleanup();
	m_Deps.System_Debug.Log("[InteractionTableSystem] INFO: Cleanup completed.");
}