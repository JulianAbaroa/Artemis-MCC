module Tables.Interaction.System;

import Common.Math.Type;
import Tables.Interaction.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;

    using Tables::Interaction::Type::Alive::AliveInteraction;
    using Tables::Interaction::Type::Alive::InteractionKind;
    using Tables::Interaction::Type::Alive::InteractionDetail;

    using Tables::Interaction::Type::Offset::k_Kind;
    using Tables::Interaction::Type::Offset::k_Detail;
    using Tables::Interaction::Type::Offset::k_TargetObjectHandle;
    using Tables::Interaction::Type::Offset::k_IsMeleeAvailable;
    using Tables::Interaction::Type::Offset::k_MeleeTargetHandle;
    using Tables::Interaction::Type::Offset::k_IsAimAvailable;
    using Tables::Interaction::Type::Offset::k_BipedBodyPart;
    using Tables::Interaction::Type::Offset::k_AimTargetHandle;
    using Tables::Interaction::Type::Offset::k_AimTargetSlotID;
    using Tables::Interaction::Type::Offset::k_AimHitLocalPosition;
}

namespace Tables::Interaction::System
{
    auto InteractionService::UpdateInteractionTable() -> void
    {
        std::uintptr_t tableBase{ m_InteractionStore.GetBase() };
        if (!tableBase)
        {
            m_InteractionStore.Publish({});
            return;
        }

        auto& reader = m_MemoryReaderService;

        AliveInteraction interaction{};

        interaction.Kind = reader.Read<InteractionKind>(tableBase, k_Kind);
        interaction.InteractionSlotID = reader.Read<InteractionDetail>(tableBase, k_Detail);
        interaction.TargetObjectHandle = reader.Read<std::uint32_t>(tableBase, k_TargetObjectHandle);

        interaction.IsMeleeAvailable = reader.Read<std::uint8_t>(tableBase, k_IsMeleeAvailable);
        interaction.MeleeTargetHandle = reader.Read<std::uint32_t>(tableBase, k_MeleeTargetHandle);

        interaction.IsAimAvailable = reader.Read<std::uint8_t>(tableBase, k_IsAimAvailable);
        interaction.ModelPart = reader.Read<std::uint8_t>(tableBase, k_BipedBodyPart);
        interaction.AimTargetHandle = reader.Read<std::uint32_t>(tableBase, k_AimTargetHandle);
        interaction.AimTargetSlotID = reader.Read<std::uint32_t>(tableBase, k_AimTargetSlotID);
        interaction.AimHitLocalPosition = reader.Read<Vec3>(tableBase, k_AimHitLocalPosition);

        m_InteractionStore.Publish(interaction);
    }

    auto InteractionService::Cleanup() -> void
    {
        m_InteractionStore.Cleanup();

        m_LogsService.Message("[InteractionService] INFO: Cleanup completed.");
    }
}