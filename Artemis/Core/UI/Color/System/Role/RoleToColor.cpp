module;

#include "External/imgui/imgui.h"

module UI.Color.System;
import :Role;

namespace UI::Color::System
{
	auto RoleToColor::FromRole(Role role) -> ImVec4
	{
		switch (role)
		{
		case Role::Vehicle:
		case Role::VehiclePart:
			return { 1.0f, 0.8f, 0.2f, 1.0f };

		case Role::WeaponPickup:
		case Role::WeaponEquipped:
			return { 0.4f, 0.8f, 1.0f, 1.0f };

		case Role::ArmorAbilityPickup:
		case Role::ArmorAbilityEquipped:
			return { 0.6f, 1.0f, 0.6f, 1.0f };

		case Role::HealthStation:
			return { 1.0f, 0.5f, 0.8f, 1.0f };

		case Role::AmmoPickup:
			return { 0.8f, 0.6f, 0.3f, 1.0f };

		case Role::Projectile:
			return { 1.0f, 0.3f, 0.3f, 1.0f };

		case Role::Biped:
			return { 0.8f, 0.6f, 1.0f, 1.0f };

		case Role::DeviceMachine:
		case Role::Teleporter:
		case Role::Lift:
		case Role::Shield:
			return { 0.5f, 0.9f, 0.9f, 1.0f };

		case Role::Explosive:
			return { 1.0f, 0.4f, 0.1f, 1.0f };

		default:
			return { 0.7f, 0.7f, 0.7f, 1.0f };

		}
	}
}