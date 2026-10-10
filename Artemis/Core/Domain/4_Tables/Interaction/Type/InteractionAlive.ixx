export module Tables.Interaction.Type:Alive;

import Common.Math.Type;
import std;

namespace
{
    using Common::Math::Type::Vec3;
}

export namespace Tables::Interaction::Type::Alive
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

    // Snapshot of the engine interaction table: the object, melee and aim targets of the local player.
    struct AliveInteraction
    {
        // Objects.
        InteractionKind Kind{ InteractionKind::None };
        InteractionDetail InteractionSlotID{ InteractionDetail::None };
        std::uint32_t TargetObjectHandle{ 0xFFFFFFFF };

        // Players: Melee.
        // note: Does not cover the extended range of the sword.
        std::uint8_t IsMeleeAvailable{ 0x00 };
        std::uint32_t MeleeTargetHandle{ 0xFFFFFFFF };

        // Players: Aim.
        // note: Follows the aim assist, so it can be set while the crosshair is not on the target.
        std::uint8_t IsAimAvailable{ 0x00 };

        // Part of the body of the aim target, as the engine reports it.
        std::uint8_t ModelPart{ 0x00 };
        std::uint32_t AimTargetHandle{ 0xFFFFFFFF };

        // note: Can hold the slot of an enemy player that is not directly under the crosshair.
        std::uint32_t AimTargetSlotID{ 0xFFFFFFFF };
        Vec3 AimHitLocalPosition{};

        static constexpr std::uint8_t k_MeleeAvailable{ 0x0E };
        static constexpr std::uint8_t k_AimAvailable{ 0x01 };

        static constexpr auto IsValidHandle(std::uint32_t handle) -> bool
        {
            return handle != 0xFFFFFFFF && handle != 0;
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
}