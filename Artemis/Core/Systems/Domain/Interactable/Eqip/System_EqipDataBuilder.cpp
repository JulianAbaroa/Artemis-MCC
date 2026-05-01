#include "pch.h"
#include "Core/Types/Domain/Domains/Interactable/EquipmentData.h"
#include "Core/Systems/Domain/Interactable/Eqip/System_EqipDataBuilder.h"
#include "Generated/Eqip/EqipObject.h"

EquipmentData System_EqipDataBuilder::BuildData(const EqipObject& eqip)
{
    EquipmentData out{};
    const auto& d = eqip.Data;

    out.TagName = eqip.TagName;
    out.Type = DeriveType(eqip);

    // Timing
    out.WarmupTime = d.WarmupTime;
    out.Duration = d.Duration;
    out.EnergyRecoveryTime = d.EnergyRecoveryTime;
    out.Charges = d.Charges;
    out.IsToggle = (d.ActivationType == 0x0);

    // Energy
    out.MinimumActivationEnergy = d.MinimumActivationEnergy;
    out.ActivationEnergyCost = d.ActivationEnergyCost;
    out.ActiveEnergyRate = d.ActiveEnergyRate;
    out.RechargeRate = d.RechargeRate;
    out.MovementSpeedDomain = d.MovementSpeedDomain;

    // Flags
    out.CannotBeActiveAirborne = (d.Flags_3 & (1u << 7)) != 0;
    out.CannotActivateAirborne = (d.Flags_3 & (1u << 8)) != 0;
    out.CannotActivateInVehicle = (d.Flags_3 & (1u << 10)) != 0;
    out.DeactivatedByFiringWeapon = (d.Flags_3 & (1u << 11)) != 0;
    out.DeactivatedByMelee = (d.Flags_3 & (1u << 12)) != 0;
    out.DeactivatedByGrenade = (d.Flags_3 & (1u << 13)) != 0;
    out.ThirdPersonWhileActive = (d.Flags_3 & (1u << 4)) != 0;

    // AI
    out.AwarenessTime = d.AwarenessTime;
    out.ObjectNoiseAdjustment = d.ObjectNoiseAdjustment;

    // Multiplayer object
    out.HasMultiplayerObject = !eqip.MultiplayerObject.empty();
    if (out.HasMultiplayerObject)
    {
        const auto& mp = eqip.MultiplayerObject[0];
        out.MultiplayerObjectType = mp.Type;
        out.BoundaryShape = mp.BoundaryShape;
        out.BoundaryWidthRadius = mp.BoundaryWidthRadius;
        out.DefaultSpawnTime = mp.DefaultSpawnTime;
        out.DefaultAbandonmentTime = mp.DefaultAbandonmentTime;
    }

    return out;
}

EquipmentType System_EqipDataBuilder::DeriveType(const EqipObject& eqip)
{
    if (!eqip.Sprint.empty())             return EquipmentType::Sprint;
    if (!eqip.Jetpack.empty())            return EquipmentType::Jetpack;
    if (!eqip.Hologram.empty())           return EquipmentType::Hologram;
    if (!eqip.InvincibilityMode.empty())  return EquipmentType::Invincibility;
    if (!eqip.MotionTrackerNoise.empty()) return EquipmentType::MotionTrackerNoise;
    if (!eqip.ProximityMine.empty())      return EquipmentType::ProximityMine;
    if (!eqip.Spawner.empty())            return EquipmentType::Spawner;
    if (!eqip.MultiplayerPowerup.empty()) return EquipmentType::Powerup;
    if (!eqip.TreeOfLife.empty())         return EquipmentType::TreeOfLife;
    if (!eqip.SpecialMove.empty())        return EquipmentType::SpecialMove;
    return EquipmentType::Unknown;
}