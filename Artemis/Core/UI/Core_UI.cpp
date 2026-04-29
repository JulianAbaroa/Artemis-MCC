#include "pch.h"
#include "Core/UI/Core_UI.h"
#include "Core/UI/UI_Main.h"
#include "Core/UI/Interface/UI_Logs.h"
#include "Core/UI/Domain/Tables/UI_ObjectTable.h"
#include "Core/UI/Domain/Tables/UI_PlayerTable.h"
#include "Core/UI/Domain/Graph/UI_ObjectGraph.h"
#include "Core/UI/Domain/Interactable/UI_Interactable.h"
#include "Core/UI/Infrastructure/UI_Settings.h"

Core_UI::Core_UI()
{
	Main = std::make_unique<UI_Main>();
	Settings = std::make_unique<UI_Settings>();
	Logs = std::make_unique<UI_Logs>();

	ObjectTable = std::make_unique<UI_ObjectTable>();
	PlayerTable = std::make_unique<UI_PlayerTable>();
	ObjectGraph = std::make_unique<UI_ObjectGraph>();
	Interactable = std::make_unique<UI_Interactable>();
}

Core_UI::~Core_UI() = default;