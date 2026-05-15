#pragma once

// Types.
#include "Core/Types/Domain/Object/LiveObject.h"

// Utils.
#include "Core/UI/Utils/ObjectSearchFilter.h"
#include "Core/UI/Utils/CopyableField.h"

#include <unordered_map>
#include <cstdint>
#include <map>

class State_ObjectTable;

class UI_ObjectTable
{
public:
	UI_ObjectTable(State_ObjectTable& stateObjectTable) :
		Sta_ObjectTable(stateObjectTable) {}
	~UI_ObjectTable() = default;

	void Draw();
	void Cleanup();

private:
	State_ObjectTable& Sta_ObjectTable;

	std::unordered_map<uint32_t, LiveObject> m_CacheObjects;
	std::map<std::string, std::vector<const LiveObject*>> m_GroupedObjects;
	ObjectSearchFilter m_SearchFilter;
	CopyableField m_CopyableField;

	// --- Card-related ---

	void DrawObjectCard(const LiveObject& object);
	void DrawCardHeader(const LiveObject& object);
	void DrawCardBaseFields(const LiveObject& object);
	void DrawCardKinematics(const LiveObject& object);

	// --- Type-related ---

	void DrawSectionBiped(const BipedObject& specific);
	void DrawSectionVehicle(const VehicleObject& specific);
	void DrawSectionWeapon(const WeaponObject& specific);
	void DrawSectionEquipment(const EquipmentObject& specific);
	void DrawShapeFields(const ZoneShape& shape);
	void DrawSectionCrate(const CrateObject& specific);
	void DrawSectionScenery(const SceneryObject& specific);
	void DrawTypeSpecificSection(const LiveObject& object);

	// --- Helpers ---

	float GetCrateSpecificHeight(const CrateObject& specific, float lineHeight);
	float GetScenerySpecificHeight(const SceneryObject& specific, float lineHeight);
	float GetSpecificHeight(const LiveObject& object, float lineHeight);
	void DrawTeleporterAllowedObjects(AllowedObjects allowed);
};