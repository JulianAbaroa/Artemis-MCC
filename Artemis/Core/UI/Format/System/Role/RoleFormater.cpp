module UI.Format.System;
import :Role;

namespace UI::Format::System
{
	auto RoleFormater::RoleToString(Role role) -> const char*
	{
		switch (role)
		{
		case Role::HealthStation:			return "HealthStation";
		case Role::Shield:					return "Shield";
		case Role::Lift:					return "Lift";
		case Role::Explosive:				return "Explosive";
		case Role::Teleporter:				return "Teleporter";
		case Role::DeviceMachine:			return "DeviceMachine";
		case Role::ObjectiveSpawn:			return "Objective Spawn";
		case Role::ObjectivePickup:			return "Objective Pickup";
		case Role::ObjectiveEquipped:		return "Objective Equiped";
		case Role::Spawn:					return "Spawn";
		case Role::Projectile:				return "Projectile";
		case Role::AmmoPickup:				return "AmmoPickup";
		case Role::ArmorAbilityEquipped:	 return "Armor Ability Equiped";
		case Role::ArmorAbilityPickup:		return "Armor Ability Pickup";
		case Role::WeaponEquipped:			 return "WeaponEquipped";
		case Role::WeaponPickup:			return "WeaponPickup";
		case Role::VehiclePart:				return "VehiclePart";
		case Role::Vehicle:					return "Vehicle";
		case Role::Biped:					return "Biped";
		default:                            return "Unknown";
		}
	}
}