#include "pch.h"

#include "Core_Hook.h"

#include "Core/States/Core_State.h"
#include "Core/Systems/Core_System.h"
#include "Core/UI/Core_UI.h"

#include "Map/Hook_BlamOpenMap.h"

#include "Gametype/Hook_GameEngineInit.h"

#include "Object/Hook_ObjectTable.h"
#include "Object/Hook_CreateObject.h"
#include "Object/Hook_ReleaseObject.h"

#include "Player/Hook_PlayerTable.h"
#include "Player/Hook_CreatePlayer.h"

#include "Interaction/Hook_InteractionTable.h"

#include "Input/Hook_GetButtonState.h"
#include "Input/Hook_GetRawInputData.h"

#include "Lifecycle/Hook_EngineInitialize.h"
#include "Lifecycle/Hook_DestroySubsystems.h"

#include "Window/Hook_WndProc.h"

#include "Render/Hook_Present.h"
#include "Render/Hook_ResizeBuffers.h"

Core_Hook::Core_Hook() = default;
Core_Hook::~Core_Hook() = default;

void Core_Hook::Initialize(Core_State& state, 
	Core_System& system, Core_UI& ui)
{
	this->InitMap(state, system);
	this->InitGametype(system);
	this->InitObject(state, system);
	this->InitPlayer(state, system);
	this->InitInteraction(state, system);
	this->InitInput(state, system, ui);
	this->InitLifecycle(state, system, ui);
	this->InitWindow(state, system, ui);
	this->InitRender(state, system, ui);
}

void Core_Hook::InitMap(Core_State& state, Core_System& system) 
{
	BlamOpenMap = std::make_unique<Hook_BlamOpenMap>(
		Hook_BlamOpenMap_Dependencies{
			.State_MapReader = *state.MapReader,
			.System_MapReader = *system.MapReader,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});
}

void Core_Hook::InitGametype(Core_System& system) 
{
	GameEngineInit = std::make_unique<Hook_GameEngineInit>(
		Hook_GameEngineInit_Dependencies{
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});
}

void Core_Hook::InitObject(Core_State& state, Core_System& system) 
{
	ObjectTable = std::make_unique<Hook_ObjectTable>(
		Hook_ObjectTable_Dependencies{
			.State_ObjectTable = *state.ObjectTable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});

	CreateGameObject = std::make_unique<Hook_CreateObject>(
		Hook_CreateObject_Dependencies{
			.System_ObjectTable = *system.ObjectTable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});

	ReleaseGameObject = std::make_unique<Hook_ReleaseObject>(
		Hook_ReleaseObject_Dependencies{
			.System_ObjectTable = *system.ObjectTable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});
}

void Core_Hook::InitPlayer(Core_State& state, Core_System& system) 
{
	PlayerTable = std::make_unique<Hook_PlayerTable>(
		Hook_PlayerTable_Dependencies{
			.State_PlayerTable = *state.PlayerTable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});

	CreatePlayer = std::make_unique<Hook_CreatePlayer>(
		Hook_CreatePlayer_Dependencies{
			.System_PlayerTable = *system.PlayerTable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});
}

void Core_Hook::InitInteraction(Core_State& state,
	Core_System& system)
{
	InteractionTable = std::make_unique<Hook_InteractionTable>(
		Hook_InteractionTable_Dependencies{
			.State_InteractionTable = *state.InteractionTable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});
}

void Core_Hook::InitInput(Core_State& state, Core_System& system,
	Core_UI& ui)
{
	GetButtonState = std::make_unique<Hook_GetButtonState>(
		Hook_GetButtonState_Dependencies{
			.State_Input = *state.Input,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});

	GetRawInputData = std::make_unique<Hook_GetRawInputData>(
		Hook_GetRawInputData_Dependencies{
			.State_Settings = *state.Settings,
			.System_Logs = *system.Logs,
			.UI_Launcher = *ui.Launcher,
		});
}

void Core_Hook::InitLifecycle(Core_State& state,
	Core_System& system, Core_UI& ui)
{
	EngineInitialize = std::make_unique<Hook_EngineInitialize>(
		Hook_EngineInitialize_Dependencies{
			.Hook_BlamOpenMap = *BlamOpenMap,
			.Hook_ObjectTable = *ObjectTable,
			.Hook_CreateObject = *CreateGameObject,
			.Hook_ReleaseObject = *ReleaseGameObject,
			.Hook_PlayerTable = *PlayerTable,
			.Hook_CreatePlayer = *CreatePlayer,
			.Hook_InteractionTable = *InteractionTable,
			.Hook_GetButtonState = *GetButtonState,
			.State_Lifecycle = *state.Lifecycle,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});

	DestroySubsystems = std::make_unique<Hook_DestroySubsystems>(
		Hook_DestroySubsystems_Dependencies{
			.Hook_BlamOpenMap = *BlamOpenMap,
			.Hook_CreateObject = *CreateGameObject,
			.Hook_ReleaseObject = *ReleaseGameObject,
			.Hook_CreatePlayer = *CreatePlayer,
			.Hook_GetButtonState = *GetButtonState,
			.State_Lifecycle = *state.Lifecycle,
			.System_MapReader = *system.MapReader,
			.System_TagGroup = *system.TagGroupReader,
			.System_ObjectTable = *system.ObjectTable,
			.System_PlayerTable = *system.PlayerTable,
			.System_InteractionTable = *system.InteractionTable,
			.System_Classifier = *system.Classifier,
			.System_ObjectGraph = *system.ObjectGraph,
			.System_PlayerGraph = *system.PlayerGraph,
			.System_Navigation = *system.Navigation,
			.System_Environment = *system.Environment,
			.System_Interactable = *system.Interactable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
			.UI_ObjectTable = *ui.ObjectTable,
			.UI_PlayerTable = *ui.PlayerTable,
			.UI_Map = *ui.Map,
		});
}

void Core_Hook::InitWindow(Core_State& state,
	Core_System& system, Core_UI& ui)
{
	WndProc = std::make_unique<Hook_WndProc>(
		Hook_WndProc_Dependencies{
			.State_MemoryScanner = *state.Memory,
			.State_Lifecycle = *state.Lifecycle,
			.State_Settings = *state.Settings,
			.System_MemoryScanner = *system.MemoryScanner,
			.System_Lifecycle = *system.Lifecycle,
			.System_Logs = *system.Logs,
			.UI_Launcher = *ui.Launcher,
			.UI_Overlay = *ui.Overlay,
		});
}

void Core_Hook::InitRender(Core_State& state,
	Core_System& system, Core_UI& ui)
{
	Present = std::make_unique<Hook_Present>(
		Hook_Present_Depedencies{
			.Hook_GetRawInputData = *GetRawInputData,
			.Hook_WndProc = *WndProc,
			.State_Render = *state.Render,
			.System_Render = *system.Render,
			.System_Logs = *system.Logs,
			.UI_Launcher = *ui.Launcher,
			.UI_Overlay = *ui.Overlay,
			.UI_ObjectTable = *ui.ObjectTable,
			.UI_PlayerTable = *ui.PlayerTable,
			.UI_Map = *ui.Map,
			.UI_Settings = *ui.Settings,
			.UI_MemoryScanner = *ui.MemoryScanner,
			.UI_Logs = *ui.Logs,
		});

	ResizeBuffers = std::make_unique<Hook_ResizeBuffers>(
		Hook_ResizeBuffers_Dependencies{
			.State_Render = *state.Render,
			.System_Render = *system.Render,
			.System_Logs = *system.Logs,
		});
}

void Core_Hook::Deinitialize()
{
	this->DeinitMap();
	this->DeinitGametype();
	this->DeinitObject();
	this->DeinitPlayer();
	this->DeinitInteraction();
	this->DeinitInput();
	this->DeinitLifecycle();
	this->DeinitWindow();
	this->DeinitRender();
}

void Core_Hook::DeinitMap() 
{
	if (BlamOpenMap) BlamOpenMap->Uninstall();
	BlamOpenMap.reset();
}

void Core_Hook::DeinitGametype() 
{
	if (GameEngineInit) GameEngineInit->Uninstall();
	GameEngineInit.reset();
}

void Core_Hook::DeinitObject() 
{
	ObjectTable.reset();

	if (CreateGameObject) CreateGameObject->Uninstall();
	CreateGameObject.reset();

	if (ReleaseGameObject) ReleaseGameObject->Uninstall();
	ReleaseGameObject.reset();
}

void Core_Hook::DeinitPlayer() 
{
	PlayerTable.reset();

	if (CreatePlayer) CreatePlayer->Uninstall();
	CreatePlayer.reset();
}

void Core_Hook::DeinitInteraction()
{
	InteractionTable.reset();
}

void Core_Hook::DeinitInput()
{
	if (GetRawInputData) GetRawInputData->Uninstall();
	GetRawInputData.reset();

	if (GetButtonState) GetButtonState->Uninstall();
	GetButtonState.reset();
}

void Core_Hook::DeinitLifecycle()
{
	if (EngineInitialize) EngineInitialize->Uninstall();
	EngineInitialize.reset();

	if (DestroySubsystems) DestroySubsystems->Uninstall();
	DestroySubsystems.reset();
}

void Core_Hook::DeinitWindow()
{
	WndProc.reset();
}

void Core_Hook::DeinitRender()
{
	if (Present) Present->Uninstall();
	Present.reset();

	if (ResizeBuffers) ResizeBuffers->Uninstall();
	ResizeBuffers.reset();
}