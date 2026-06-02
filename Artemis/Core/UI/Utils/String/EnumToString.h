#pragma once

#include "Core/Types/Object/Specific/WeaponObject.h"
#include "Core/Types/Object/Specific/VehicleObject.h"
#include "Core/Types/Object/Specific/BipedObject.h"
#include "Core/Types/Object/Specific/EquipmentObject.h"
#include "Core/Types/Object/Specific/CrateObject.h"
#include "Core/Types/Object/Specific/SceneryObject.h"
#include "Core/Types/Navigation/NavigationTypes.h"

class EnumToString
{
public:
	static const char* ActionStateToString(ActionState state);
	static const char* VehicleTypeToString(VehicleType type);
	static const char* TeamToString(Team team);
	static const char* ZoomLevelToString(ZoomLevel zoom);
	static const char* CrateTypeToString(CrateType type);
	static const char* SceneryTypeToString(SceneryType type);
	static const char* ShapeTypeToString(ShapeType type);
	static const char* AngleTypeToString(AngleType type);
	static const char* ForceTypeToString(ForceType type);
	static const char* DestructibleTypeToString(DestructibleType type);
	static const char* TeleporterTypeToString(TeleporterType type);
};