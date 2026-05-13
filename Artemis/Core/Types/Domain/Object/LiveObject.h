#pragma once

// Note: It seems the grenades are not saved on the biped object.
// Note: It seems the object table has lazy deinitialization, 
// meaning that the objects data are not fully cleared from memory 
// until another, new object takes their memory space.

#include "ObjectClass.h"
#include "ObjectProfile.h"
#include "Specific/BipedObject.h"
#include "Specific/EquipmentObject.h"
#include "Specific/VehicleObject.h"
#include "Specific/WeaponObject.h"
#include "Specific/ProjectileObject.h"
#include "Specific/CrateObject.h"
#include "Specific/SceneryObject.h"
#include <variant>
#include <cstdint>
#include <string>
#include <array>

// TODO: SceneryObject, ProjectileObject, 
// DeviceMachineObject, DeviceControlObject.

using SpecificObject = std::variant<std::monostate, 
	EquipmentObject, VehicleObject, WeaponObject, BipedObject,
	CrateObject, SceneryObject, ProjectileObject>;

// Representation of an object inside the object table.
struct LiveObject
{
	uint32_t DatumIndex{};
	uintptr_t Address{};

	std::string Class{};
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

	ObjectProfile Profile{};
	ObjectClass Type = ObjectClass::Invalid;
	SpecificObject SpecificObject{};
};