#pragma once

#include "ObjectProfile.h"
#include "Specific/BipedObject.h"
#include "Specific/EquipmentObject.h"
#include "Specific/VehicleObject.h"
#include "Specific/WeaponObject.h"
#include "Specific/ProjectileObject.h"
#include "Specific/CrateObject.h"
#include "Specific/SceneryObject.h"
#include <variant>

// TODO: DeviceControlObject.

using SpecificObject = std::variant<std::monostate, 
	EquipmentObject, VehicleObject, WeaponObject, BipedObject,
	CrateObject, SceneryObject, ProjectileObject>;

struct LiveObject
{
	uint32_t DatumIndex{};
	uintptr_t Address{};

	std::string FourCC{};
	std::string TagName{};

	uint32_t Handle{};
	uint32_t NextSiblingHandle{};
	uint32_t ChildHandle{};
	uint32_t ParentHandle{};

	std::array<float, 3> Position{};
	std::array<float, 3> Forward{};
	std::array<float, 3> Up{};
	std::array<float, 3> LinearVelocity{};
	std::array<float, 3> AngularVelocity{};

	float CurrentRadius{};
	float DamageReceived{};

	uint8_t HlmtVariant = 0;

	ObjectProfile Profile{};
	SpecificObject SpecificObject{};
};