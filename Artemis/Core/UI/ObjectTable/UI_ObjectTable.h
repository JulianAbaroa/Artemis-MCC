#pragma once

#include "Core/Types/Object/LiveObject.h"

#include "Core/UI/Launcher/UI_Tab.h"

#include "Core/UI/Utils/SearchFilter/ObjectSearchFilter.h"
#include "Core/UI/Utils/Field/CopyableField.h"

#include <unordered_map>
#include <cstdint>
#include <map>

class State_ObjectTable;

class UI_ObjectTable : public UI_Tab
{
public:
	UI_ObjectTable(State_ObjectTable& stateObjectTable) :
		State_ObjectTable(stateObjectTable) {}
	~UI_ObjectTable() = default;

	void Draw();
	void Cleanup();

private:
	State_ObjectTable& State_ObjectTable;

	std::unordered_map<uint32_t, LiveObject> m_CacheObjects;
	std::map<std::string, std::vector<const LiveObject*>> m_GroupedObjects;
	ObjectSearchFilter m_SearchFilter;
	CopyableField m_CopyableField;

	void DrawObjectCard(const LiveObject& object);
	void DrawCardHeader(const LiveObject& object);
	void DrawCardFields(const LiveObject& object);
};