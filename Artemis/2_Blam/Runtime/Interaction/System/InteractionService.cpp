module Blam.Runtime.Interaction.System;

import Common.Engine.Type;
import Common.Math.Type;
import Blam.Layout.Interaction.Type;
import Blam.Layout.Player.Type;
import Blam.Layout.Snapshot.Type;
import Blam.Runtime.Interaction.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;

    using Blam::Runtime::Interaction::Type::Alive::LocalInteractions;
    using Blam::Runtime::Interaction::Type::Alive::InteractionKind;
    using Blam::Runtime::Interaction::Type::Alive::InteractionDetail;

    using Blam::Layout::Snapshot::Type::RegionId;

    using Blam::Layout::Interaction::Type::Offset::k_Kind;
    using Blam::Layout::Interaction::Type::Offset::k_Detail;
    using Blam::Layout::Interaction::Type::Offset::k_TargetObjectHandle;
    using Blam::Layout::Interaction::Type::Offset::k_IsMeleeAvailable;
    using Blam::Layout::Interaction::Type::Offset::k_MeleeTargetHandle;
    using Blam::Layout::Interaction::Type::Offset::k_IsAimAvailable;
    using Blam::Layout::Interaction::Type::Offset::k_BipedBodyPart;
    using Blam::Layout::Interaction::Type::Offset::k_AimTargetHandle;
    using Blam::Layout::Interaction::Type::Offset::k_AimTargetSlotID;
    using Blam::Layout::Interaction::Type::Offset::k_AimHitLocalPosition;
    using Blam::Layout::Interaction::Type::Offset::k_OutputUserStride;
    using Blam::Layout::Interaction::Type::Offset::k_Interaction;

    using Blam::Layout::Player::Type::Mapping::k_PlayerInputUser;
    using Blam::Layout::Player::Type::Mapping::k_OutputUserPlayer;
    using Blam::Layout::Player::Type::Mapping::k_EntryStride;

    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_NoHandle;
    using Common::Engine::Type::k_HandleIndexMask;
}

namespace Blam::Runtime::Interaction::System
{
    auto InteractionService::UpdateInteractionTable(const Snapshot& snapshot) -> void
    {
        LocalInteractions interactions{};

        if (!snapshot.IsCaptured(RegionId::Interaction) || !snapshot.IsCaptured(RegionId::PlayerMapping))
        {
            m_InteractionStore.Publish(interactions);
            return;
        }

        for (std::size_t view{ 0 }; view < k_MaxLocalUsers; ++view)
        {
            const std::uint32_t playerHandle{ snapshot.Read<std::uint32_t>(RegionId::PlayerMapping, k_OutputUserPlayer + (view * k_EntryStride)) };
            if (playerHandle == k_NoHandle) continue;

            const std::int32_t localIndex{ snapshot.Read<std::int32_t>(RegionId::PlayerMapping,
                k_PlayerInputUser + ((playerHandle & k_HandleIndexMask) * k_EntryStride)) };

            if (localIndex < 0 || static_cast<std::size_t>(localIndex) >= k_MaxLocalUsers) continue;

            AliveInteraction& interaction{ interactions[static_cast<std::size_t>(localIndex)] };
            interaction = this->ReadInteraction(snapshot, (view * k_OutputUserStride) + k_Interaction);
            interaction.PlayerHandle = playerHandle;
        }

        m_InteractionStore.Publish(interactions);
    }

    auto InteractionService::ReadInteraction(const Snapshot& snapshot, std::size_t interactionOffset) -> AliveInteraction
    {
        AliveInteraction interaction{};

        interaction.Kind = snapshot.Read<InteractionKind>(RegionId::Interaction, interactionOffset + k_Kind);
        interaction.InteractionSlotID = snapshot.Read<InteractionDetail>(RegionId::Interaction, interactionOffset + k_Detail);
        interaction.TargetObjectHandle = snapshot.Read<std::uint32_t>(RegionId::Interaction, interactionOffset + k_TargetObjectHandle);

        interaction.IsMeleeAvailable = snapshot.Read<std::uint8_t>(RegionId::Interaction, interactionOffset + k_IsMeleeAvailable);
        interaction.MeleeTargetHandle = snapshot.Read<std::uint32_t>(RegionId::Interaction, interactionOffset + k_MeleeTargetHandle);

        interaction.IsAimAvailable = snapshot.Read<std::uint8_t>(RegionId::Interaction, interactionOffset + k_IsAimAvailable);
        interaction.ModelPart = snapshot.Read<std::uint8_t>(RegionId::Interaction, interactionOffset + k_BipedBodyPart);
        interaction.AimTargetHandle = snapshot.Read<std::uint32_t>(RegionId::Interaction, interactionOffset + k_AimTargetHandle);
        interaction.AimTargetSlotID = snapshot.Read<std::uint32_t>(RegionId::Interaction, interactionOffset + k_AimTargetSlotID);
        interaction.AimHitLocalPosition = snapshot.Read<Vec3>(RegionId::Interaction, interactionOffset + k_AimHitLocalPosition);

        return interaction;
    }

    auto InteractionService::Cleanup() -> void
    {
        m_InteractionStore.Cleanup();

        m_Log.Message("[InteractionService] INFO: Cleanup completed.");
    }
}