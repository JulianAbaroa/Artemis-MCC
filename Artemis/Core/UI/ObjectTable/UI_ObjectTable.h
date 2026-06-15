#pragma once

#include "Core/Types/Sources/Tables/Object/LiveObject.h"

#include "Core/States/Sources/Tables/Object/State_ObjectTable.h"

#include "Core/UI/Launcher/UI_Tab.h"

#include "Core/UI/Utils/SearchFilter/ObjectSearchFilter.h"
#include "Core/UI/Utils/Field/CopyableField.h"

#include <memory>
#include <map>

struct Tick;

class State_ObjectTable;

class UI_ObjectTable : public UI_Tab
{
public:
	UI_ObjectTable(State_ObjectTable& stateObjectTable) :
		State_ObjectTable(stateObjectTable) {}
	~UI_ObjectTable() = default;

	void Draw(std::shared_ptr<const Tick> tick);

	void Cleanup();

private:
	State_ObjectTable& State_ObjectTable;

	std::shared_ptr<const ObjectTable> m_ObjectTable;
	std::map<std::string, std::vector<const LiveObject*>> m_GroupedObjects;

	ObjectSearchFilter m_SearchFilter;
	CopyableField m_CopyableField;

	void DrawObjectCard(const LiveObject& object);
	void DrawCardHeader(const LiveObject& object);
	void DrawCardFields(const LiveObject& object);
};