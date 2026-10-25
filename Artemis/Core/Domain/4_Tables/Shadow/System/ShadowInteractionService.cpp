module Tables.Shadow.System;
import :Interaction;

import Common.Engine.Type;
import Common.Math.Type;
import Layout.Interaction.Type;
import Layout.Player.Type;
import Tables.Interaction.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;

    using Tables::Interaction::Type::Alive::LocalInteractions;
    using Tables::Interaction::Type::Alive::InteractionKind;
    using Tables::Interaction::Type::Alive::InteractionDetail;

    using Layout::Interaction::Type::Offset::k_Kind;
    using Layout::Interaction::Type::Offset::k_Detail;
    using Layout::Interaction::Type::Offset::k_TargetObjectHandle;
    using Layout::Interaction::Type::Offset::k_IsMeleeAvailable;
    using Layout::Interaction::Type::Offset::k_MeleeTargetHandle;
    using Layout::Interaction::Type::Offset::k_IsAimAvailable;
    using Layout::Interaction::Type::Offset::k_BipedBodyPart;
    using Layout::Interaction::Type::Offset::k_AimTargetHandle;
    using Layout::Interaction::Type::Offset::k_AimTargetSlotID;
    using Layout::Interaction::Type::Offset::k_AimHitLocalPosition;
    using Layout::Interaction::Type::Offset::k_OutputUserStride;
    using Layout::Interaction::Type::Offset::k_Interaction;

    using Layout::Player::Type::Mapping::k_PlayerInputUser;
    using Layout::Player::Type::Mapping::k_OutputUserPlayer;
    using Layout::Player::Type::Mapping::k_EntryStride;

    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_HandleIndexMask;
}

namespace Tables::Shadow::System
{
    auto ShadowInteractionService::UpdateInteractionTable() -> void
    {
        const std::uintptr_t tableBase{ m_CaptureStore.GetInteractionTableBase() };
        const std::uintptr_t mappingBase{ m_CaptureStore.GetMappingBase() };

        LocalInteractions interactions{};

        if (!tableBase || !mappingBase)
        {
            m_InteractionStore.Publish(interactions);
            return;
        }

        auto& reader = m_MemoryReaderService;

        for (std::size_t view{ 0 }; view < k_MaxLocalUsers; ++view)
        {
            const std::uint32_t playerHandle{ reader.Read<std::uint32_t>(mappingBase, k_OutputUserPlayer + (view * k_EntryStride)) };
            if (playerHandle == k_NoHandle) continue;

            const std::int32_t localIndex{ reader.Read<std::int32_t>(mappingBase,
                k_PlayerInputUser + ((playerHandle & k_HandleIndexMask) * k_EntryStride)) };

            if (localIndex < 0 || static_cast<std::size_t>(localIndex) >= k_MaxLocalUsers) continue;

            AliveInteraction& interaction{ interactions[static_cast<std::size_t>(localIndex)] };
            interaction = this->ReadInteraction(tableBase + (view * k_OutputUserStride) + k_Interaction);
            interaction.PlayerHandle = playerHandle;
        }

        m_InteractionStore.Publish(interactions);
    }

    auto ShadowInteractionService::ReadInteraction(std::uintptr_t interactionBase) -> AliveInteraction
    {
        auto& reader = m_MemoryReaderService;

        AliveInteraction interaction{};

        interaction.Kind = reader.Read<InteractionKind>(interactionBase, k_Kind);
        interaction.InteractionSlotID = reader.Read<InteractionDetail>(interactionBase, k_Detail);
        interaction.TargetObjectHandle = reader.Read<std::uint32_t>(interactionBase, k_TargetObjectHandle);

        interaction.IsMeleeAvailable = reader.Read<std::uint8_t>(interactionBase, k_IsMeleeAvailable);
        interaction.MeleeTargetHandle = reader.Read<std::uint32_t>(interactionBase, k_MeleeTargetHandle);

        interaction.IsAimAvailable = reader.Read<std::uint8_t>(interactionBase, k_IsAimAvailable);
        interaction.ModelPart = reader.Read<std::uint8_t>(interactionBase, k_BipedBodyPart);
        interaction.AimTargetHandle = reader.Read<std::uint32_t>(interactionBase, k_AimTargetHandle);
        interaction.AimTargetSlotID = reader.Read<std::uint32_t>(interactionBase, k_AimTargetSlotID);
        interaction.AimHitLocalPosition = reader.Read<Vec3>(interactionBase, k_AimHitLocalPosition);

        return interaction;
    }

    auto ShadowInteractionService::Cleanup() -> void
    {
        m_InteractionStore.Cleanup();

        m_Log.Message("[ShadowInteractionService] INFO: Cleanup completed.");
    }
}