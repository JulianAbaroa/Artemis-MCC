#include "pch.h"
#include "Core/Types/Domain/Domains/Interactable/EquipmentData.h"
#include "Core/Systems/Domain/Interactable/Eqip/System_EqipDataBuilder.h"
#include "Generated/Eqip/EqipObject.h"

EquipmentData System_EqipDataBuilder::BuildData(const EqipObject& eqip)
{
    EquipmentData out{};

    out.TagName = eqip.TagName;
    out.Type = this->DeriveType(eqip);

    // Use and duration.
    out.WarmupTime = eqip.Data.WarmupTime;
    out.Duration = eqip.Data.Duration;
    out.CooldownTime = eqip.Data.CooldownTime;
    out.Charges = eqip.Data.Charges;
    out.IsToggle = (eqip.Data.ActivationType == 0x0); // 0=Toggle, 1=Hold

    // Energy.
    out.MinimumActivationEnergy = eqip.Data.MinimumActivationEnergy;
    out.ActiveEnergyRate = eqip.Data.ActiveEnergyRate;
    out.RechargeRate = eqip.Data.RechargeRate;

    // AI behavior.
    // Flags_3 bit 1 = Equipment Is Dangerous To AI
    // Flags_3 bit 2 = Never Dropped By AI
    out.DangerRadius = eqip.Data.DangerRadius;
    out.IsDangerousToAI = (eqip.Data.Flags_3 & (1u << 1)) != 0;
    out.NeverDroppedByAI = (eqip.Data.Flags_3 & (1u << 2)) != 0;

    return out;
}

EquipmentType System_EqipDataBuilder::DeriveType(const EqipObject& eqip)
{
    // We derive the type by the presence of specific sub-blocks.
    // Order matters : we check the most specific ones first.
    if (!eqip.Sprint.empty())              return EquipmentType::Sprint;
    if (!eqip.Jetpack.empty())             return EquipmentType::Jetpack;
    if (!eqip.HealthPack.empty())          return EquipmentType::HealthPack;
    if (!eqip.AmmoPack.empty())            return EquipmentType::AmmoPack;
    if (!eqip.Hologram.empty())            return EquipmentType::Hologram;
    if (!eqip.InvincibilityMode.empty())   return EquipmentType::Invincibility;
    if (!eqip.RepulsorField.empty())       return EquipmentType::RepulsorField;
    if (!eqip.ProximityMine.empty())       return EquipmentType::ProximityMine;
    if (!eqip.MotionTrackerNoise.empty())  return EquipmentType::MotionTrackerNoise;
    if (!eqip.PowerFist.empty())           return EquipmentType::PowerFist;
    if (!eqip.Spawner.empty())             return EquipmentType::Spawner;
    if (!eqip.MultiplayerPowerup.empty())  return EquipmentType::Powerup;
    if (!eqip.SuperJump.empty())           return EquipmentType::SuperJump;
    return EquipmentType::Unknown;
}