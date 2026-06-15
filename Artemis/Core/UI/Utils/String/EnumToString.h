#pragma once

#include "Core/Types/Sources/Tables/Object/Specific/WeaponObject.h"
#include "Core/Types/Sources/Tables/Object/Specific/VehicleObject.h"
#include "Core/Types/Sources/Tables/Object/Specific/BipedObject.h"
#include "Core/Types/Sources/Tables/Object/Specific/EquipmentObject.h"
#include "Core/Types/Sources/Tables/Object/Specific/CrateObject.h"
#include "Core/Types/Sources/Tables/Object/Specific/SceneryObject.h"
#include "Core/Types/Sources/Tables/Interaction/LiveInteraction.h"
#include "Core/Types/Structure/Classified/Classified.h"
#include "Core/Types/Egocentric/Affordance/Affordance.h"
#include "Core/Types/Environment/Fixtures/Fixtures.h"

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
	static const char* RoleToString(ObjectRole role);
	static const char* BehaviorToString(AffordanceBehavior behavior);
	static const char* ActivationToString(AffordanceActivation act);
	static const char* InteractionTypeToString(InteractionType type);
	static const char* InteractionDetailToString(InteractionType type, InteractionDetail detail);
};