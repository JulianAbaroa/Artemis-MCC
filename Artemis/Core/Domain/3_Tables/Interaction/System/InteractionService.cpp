module Tables.Interaction.System;

import Common.Math.Type;
import Tables.Interaction.Type;
import std;

namespace
{
	namespace Offset = Tables::Interaction::Type::Offset;

	using Vec3 = Common::Math::Type::Vec3;
	using AliveInteraction = Tables::Interaction::Type::Alive::Interaction;
	using InteractionKind = Tables::Interaction::Type::Alive::Kind;
	using InteractionDetail = Tables::Interaction::Type::Alive::Detail;
}

namespace Tables::Interaction::System
{
	auto InteractionService::UpdateInteractionTable() -> void
	{
		std::uintptr_t tableBase = m_InteractionStore.GetBase();
		if (!tableBase)
		{
			m_InteractionStore.Publish({});
			return;
		}

		auto& reader = m_MemoryReaderService;
	
		AliveInteraction interaction;
	
		interaction.Kind = reader.Read<InteractionKind>(tableBase, Offset::Kind);
		interaction.InteractionSlotID = reader.Read<InteractionDetail>(tableBase, Offset::Detail);
		interaction.TargetObjectHandle = reader.Read<std::uint32_t>(tableBase, Offset::TargetObjectHandle);
	
		interaction.IsMeleeAvailable = reader.Read<std::uint8_t>(tableBase, Offset::IsMeleeAvailable);
		interaction.MeleeTargetHandle = reader.Read<std::uint32_t>(tableBase, Offset::MeleeTargetHandle);
	
		interaction.IsAimAvailable = reader.Read<std::uint8_t>(tableBase, Offset::IsAimAvailable);
		interaction.ModelPart = reader.Read<std::uint8_t>(tableBase, Offset::BipedBodyPart);
		interaction.AimTargetHandle = reader.Read<std::uint32_t>(tableBase, Offset::AimTargetHandle);
		interaction.AimTargetSlotID = reader.Read<std::uint32_t>(tableBase, Offset::AimTargetSlotID);
		interaction.AimHitLocalPosition = reader.Read<Vec3>(tableBase, Offset::AimHitLocalPosition);
	
		m_InteractionStore.Publish(interaction);
	}

	auto InteractionService::Cleanup() -> void
	{
		m_InteractionStore.Cleanup();

		m_LogsService.Message("[InteractionService] INFO: Cleanup completed.");
	}
}