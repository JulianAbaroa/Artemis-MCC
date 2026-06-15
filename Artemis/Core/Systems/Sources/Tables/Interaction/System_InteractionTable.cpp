#include "pch.h"

#include "System_InteractionTable.h"

#include "Core/Types/Sources/Tables/Interaction/InteractionOffsets.h"

#include "Core/States/Sources/Tables/Interaction/State_InteractionTable.h"

#include "Core/Systems/Other/Memory/Reader/System_MemoryReader.h"
#include "Core/Systems/Other/Logs/System_Logs.h"

void System_InteractionTable::UpdateInteractionTable()
{
	uintptr_t tableBase = m_Deps.State_InteractionTable.GetBase();
	if (!tableBase)
	{
		m_Deps.State_InteractionTable.Publish({});
		return;
	}

	namespace Interaction = InteractionOffsets;
	
	auto& reader = m_Deps.System_MemoryReader;
	
	LiveInteraction interaction;
	
	interaction.Type = reader.Read<InteractionType>(tableBase, Interaction::InteractionType);
	interaction.InteractionSlotID = reader.Read<InteractionDetail>(tableBase, Interaction::InteractionDetail);
	interaction.TargetObjectHandle = reader.Read<uint32_t>(tableBase, Interaction::TargetObjectHandle);
	
	interaction.IsMeleeAvailable = reader.Read<uint8_t>(tableBase, Interaction::IsMeleeAvailable);
	interaction.MeleeTargetHandle = reader.Read<uint32_t>(tableBase, Interaction::MeleeTargetHandle);
	
	interaction.IsAimAvailable = reader.Read<uint8_t>(tableBase, Interaction::IsAimAvailable);
	interaction.ModelPart = reader.Read<uint8_t>(tableBase, Interaction::BipedBodyPart);
	interaction.AimTargetHandle = reader.Read<uint32_t>(tableBase, Interaction::AimTargetHandle);
	interaction.AimTargetSlotID = reader.Read<uint32_t>(tableBase, Interaction::AimTargetSlotID);
	interaction.AimHitLocalPosition = reader.ReadArray<float, 3>(tableBase, Interaction::AimHitLocalPosition);
	
	m_Deps.State_InteractionTable.Publish(interaction);
}

void System_InteractionTable::Cleanup()
{
	m_Deps.State_InteractionTable.Cleanup();

	m_Deps.System_Logs.Log("[InteractionTable] INFO:"
		" Cleanup completed.");
}