export module Blam.Runtime.Interaction.Type:Alive;

import Common.Engine.Type;
import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;

    using Common::Engine::Type::k_MaxLocalUsers;
    using Common::Engine::Type::k_MaxPlayers;
    using Common::Engine::Type::k_NoHandle;
}

export namespace Blam::Runtime::Interaction::Type::Alive
{
    // Kind of object interaction the engine offers to the player.
    enum class InteractionKind : std::uint8_t
    {
        None = 0x00,
        GrabWeapon = 0x02,
        GrabArmorAbility = 0x03,
        TakeHealthStation = 0x04,
        EnterVehicle = 0x06,
        Hijack = 0x09,
        GrabObjective = 0x0A,
    };

    // Detail of an interaction. Which values apply depends on the kind.
    // note: The seats apply to EnterVehicle and Hijack. GrabWeapon and ChangeWeapon apply to GrabWeapon.
    enum class InteractionDetail : std::uint8_t
    {
        None = 0xFF,
        ZeroSeat = 0x00,
        FirstSeat = 0x01,
        SecondSeat = 0x02,
        ThirdSeat = 0x03,
        FourthSeat = 0x04,
        FifthSeat = 0x05,
        GrabWeapon = 0x01,
        ChangeWeapon = 0x02
    };

    // The one object the engine picked for a player to use: the first record of an interaction (kind, detail and object), whichever
    // source it was read from (the table of a local player or the action the simulation applied to any player).
    struct ObjectSelection
    {
        InteractionKind Kind{ InteractionKind::None };
        InteractionDetail Detail{ InteractionDetail::None };
        std::uint32_t TargetHandle{ k_NoHandle };
    };

    // The selection of every player that had an applied action in the tick, by player table index (the low 16 bits of the handle).
    // Players without an action are None / None / k_NoHandle.
    struct ObjectSelections : std::array<ObjectSelection, k_MaxPlayers> {};

    // Snapshot of the engine interaction table of one local player: its object, melee and aim targets.
    struct AliveInteraction
    {
        // Player the interaction belongs to, or 0xFFFFFFFF when the local player has no view.
        std::uint32_t PlayerHandle{ k_NoHandle };

        // Objects.
        InteractionKind Kind{ InteractionKind::None };
        InteractionDetail InteractionSlotID{ InteractionDetail::None };
        std::uint32_t TargetObjectHandle{ k_NoHandle };

        // Players: Melee.
        // note: Does not cover the extended range of the sword.
        std::uint8_t IsMeleeAvailable{ 0x00 };
        std::uint32_t MeleeTargetHandle{ k_NoHandle };

        // Players: Aim.
        // note: Follows the aim assist, so it can be set while the crosshair is not on the target.
        std::uint8_t IsAimAvailable{ 0x00 };

        // Part of the body of the aim target, as the engine reports it.
        std::uint8_t ModelPart{ 0x00 };
        std::uint32_t AimTargetHandle{ k_NoHandle };

        // note: Can hold the slot of an enemy player that is not directly under the crosshair.
        std::uint32_t AimTargetSlotID{ k_NoHandle };
        Vec3 AimHitLocalPosition{};

        static constexpr std::uint8_t k_MeleeAvailable{ 0x0E };
        static constexpr std::uint8_t k_AimAvailable{ 0x01 };

        static constexpr auto IsValidHandle(std::uint32_t handle) -> bool
        {
            return handle != k_NoHandle && handle != 0;
        }

        auto HasObjectTarget() const -> bool
        {
            return this->IsValidHandle(TargetObjectHandle);
        }

        // note: A true result means the aim assist has a target, not that the crosshair is on it.
        auto HasMeleeTarget() const -> bool
        {
            return IsMeleeAvailable == k_MeleeAvailable && this->IsValidHandle(MeleeTargetHandle);
        }

        // note: A true result means the aim assist has a target, not that the crosshair is on it.
        auto HasAimTarget() const -> bool
        {
            return IsAimAvailable == k_AimAvailable && this->IsValidHandle(AimTargetHandle);
        }
    };

    // Interaction of every local player, by local index (split-screen input user).
    struct LocalInteractions : std::array<AliveInteraction, k_MaxLocalUsers> {};
}