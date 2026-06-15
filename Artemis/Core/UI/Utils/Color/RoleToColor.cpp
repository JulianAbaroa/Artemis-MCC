#include "pch.h"

#include "RoleToColor.h"

#include "Core/Types/Structure/Classified/Classified.h"

#include "External/imgui/imgui.h"

ImVec4 RoleToColor::FromRole(ObjectRole role)
{
	switch (role)
	{
	case ObjectRole::Vehicle:
	case ObjectRole::VehiclePart:
		return { 1.0f, 0.8f, 0.2f, 1.0f };
	case ObjectRole::WeaponPickup:
	case ObjectRole::WeaponEquipped:
		return { 0.4f, 0.8f, 1.0f, 1.0f };
	case ObjectRole::ArmorAbilityPickup:
	case ObjectRole::ArmorAbilityEquipped:
		return { 0.6f, 1.0f, 0.6f, 1.0f };
	case ObjectRole::HealthStation:
		return { 1.0f, 0.5f, 0.8f, 1.0f };
	case ObjectRole::AmmoPickup:
		return { 0.8f, 0.6f, 0.3f, 1.0f };
	case ObjectRole::Projectile:
		return { 1.0f, 0.3f, 0.3f, 1.0f };
	case ObjectRole::Biped:
		return { 0.8f, 0.6f, 1.0f, 1.0f };
	case ObjectRole::DeviceMachine:
	case ObjectRole::Teleporter:
	case ObjectRole::Lift:
	case ObjectRole::Shield:
		return { 0.5f, 0.9f, 0.9f, 1.0f };
	case ObjectRole::Explosive:
		return { 1.0f, 0.4f, 0.1f, 1.0f };
	default:
		return { 0.7f, 0.7f, 0.7f, 1.0f };
	}
}