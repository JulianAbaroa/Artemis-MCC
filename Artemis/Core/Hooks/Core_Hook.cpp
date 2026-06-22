#include "pch.h"

#include "Core_Hook.h"

#include "Core/States/Core_State.h"
#include "Core/Systems/Core_System.h"
#include "Core/UI/Core_UI.h"

#include "Sources/Map/Hook_BlamOpenMap.h"
#include "Sources/Object/Hook_ObjectTable.h"
#include "Sources/Object/Hook_CreateObject.h"
#include "Sources/Object/Hook_ReleaseObject.h"
#include "Sources/Object/BoneMatrix/Hook_InitRootNode.h"
#include "Sources/Player/Hook_PlayerTable.h"
#include "Sources/Player/Hook_CreatePlayer.h"
#include "Sources/Interaction/Hook_InteractionTable.h"

#include "Other/Input/Hook_GetButtonState.h"
#include "Other/Input/Hook_GetRawInputData.h"
#include "Other/Lifecycle/Hook_EngineInitialize.h"
#include "Other/Lifecycle/Hook_DestroySubsystems.h"
#include "Other/Render/Hook_Present.h"
#include "Other/Render/Hook_ResizeBuffers.h"
#include "Other/Window/Hook_WndProc.h"

#include "Tick/Hook_SimulationTicks.h"

Core_Hook::Core_Hook() = default;
Core_Hook::~Core_Hook() = default;

void Core_Hook::Initialize(Core_State& state, 
	Core_System& system, Core_UI& ui)
{
	this->InitSources(state, system);
	this->InitTick(state, system, ui);
	this->InitOther(state, system, ui);
}

void Core_Hook::InitSources(Core_State& state, Core_System& system) 
{
	BlamOpenMap = std::make_unique<Hook_BlamOpenMap>(
		Hook_BlamOpenMap_Dependencies{
			.State_MapReader = *state.MapReader,
			.System_MapReader = *system.MapReader,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});

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

	InitRootNode = std::make_unique<Hook_InitRootNode>(
		Hook_InitRootNode_Dependencies{
			.State_BoneOffsets = *state.BoneOffsets,
			.System_ObjectTable = *system.ObjectTable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});

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

	InteractionTable = std::make_unique<Hook_InteractionTable>(
		Hook_InteractionTable_Dependencies{
			.State_InteractionTable = *state.InteractionTable,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});
}

void Core_Hook::InitTick(Core_State& state,
	Core_System& system, Core_UI& ui)
{
	SimulationTicks = std::make_unique<Hook_SimulationTicks>(
		Hook_SimulationTicks_Deps{
			.State_Telemetry = *state.Telemetry,
			.State_Lifecycle = *state.Lifecycle,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
		});
}

void Core_Hook::InitOther(Core_State& state, Core_System& system,
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
			.State_FlyCamera = *state.FlyCamera,
			.System_Logs = *system.Logs,
			.UI_Launcher = *ui.Launcher,
		});

	EngineInitialize = std::make_unique<Hook_EngineInitialize>(
		Hook_EngineInitialize_Dependencies{
			.Hook_BlamOpenMap = *BlamOpenMap,
			.Hook_ObjectTable = *ObjectTable,
			.Hook_CreateObject = *CreateGameObject,
			.Hook_ReleaseObject = *ReleaseGameObject,
			.Hook_InitRootNode = *InitRootNode,
			.Hook_PlayerTable = *PlayerTable,
			.Hook_CreatePlayer = *CreatePlayer,
			.Hook_InteractionTable = *InteractionTable,
			.Hook_SimulationTicks = *SimulationTicks,
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
			.Hook_InitRootNode = *InitRootNode,
			.Hook_CreatePlayer = *CreatePlayer,
			.Hook_SimulationTicks = *SimulationTicks,
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
			.System_WorldBuilder = *system.WorldBuilder,
			.System_Collidables = *system.Collidables,
			.System_StatsBuilder = *system.StatsBuilder,
			.System_VitalityBuilder = *system.VitalityBuilder,
			.System_Vitality = *system.Vitality,
			.System_Self = *system.Self,
			.System_Fixtures = *system.Fixtures,
			.System_Affordances = *system.Affordances,
			.System_MapRenderer = *system.MapRenderer,
			.System_AOBScanner = *system.AOBScanner,
			.System_Logs = *system.Logs,
			.UI_ObjectTable = *ui.ObjectTable,
			.UI_PlayerTable = *ui.PlayerTable,
		});

	WndProc = std::make_unique<Hook_WndProc>(
		Hook_WndProc_Dependencies{
			.State_MemoryScanner = *state.Memory,
			.State_Lifecycle = *state.Lifecycle,
			.State_Settings = *state.Settings,
			.State_FlyCamera = *state.FlyCamera,
			.State_Selection = *state.Selection,
			.State_OverlayMode = *state.OverlayMode,
			.System_MemoryScanner = *system.MemoryScanner,
			.System_Lifecycle = *system.Lifecycle,
			.System_Logs = *system.Logs,
			.UI_Launcher = *ui.Launcher,
			.UI_Overlay = *ui.Overlay,
		});

	Present = std::make_unique<Hook_Present>(
		Hook_Present_Deps{
			.Hook_GetRawInputData = *GetRawInputData,
			.Hook_WndProc = *WndProc,
			.State_WorldBuilder = *state.WorldBuilder,
			.State_Tick = *state.Tick,
			.State_Render = *state.Render,
			.State_Telemetry = *state.Telemetry,
			.State_Settings = *state.Settings,
			.System_Render = *system.Render,
			.System_MapRenderer = *system.MapRenderer,
			.System_Logs = *system.Logs,
			.UI_Launcher = *ui.Launcher,
			.UI_Overlay = *ui.Overlay,
			.UI_ObjectTable = *ui.ObjectTable,
			.UI_PlayerTable = *ui.PlayerTable,
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
	this->DeinitSources();
	this->DeinitTick();
	this->DeinitOther();
}

void Core_Hook::DeinitSources() 
{
	if (BlamOpenMap) BlamOpenMap->Uninstall();
	BlamOpenMap.reset();

	ObjectTable.reset();

	if (CreateGameObject) CreateGameObject->Uninstall();
	CreateGameObject.reset();

	if (ReleaseGameObject) ReleaseGameObject->Uninstall();
	ReleaseGameObject.reset();

	PlayerTable.reset();

	if (CreatePlayer) CreatePlayer->Uninstall();
	CreatePlayer.reset();

	InteractionTable.reset();
}

void Core_Hook::DeinitTick()
{
	if (SimulationTicks) SimulationTicks->Uninstall();
	SimulationTicks.reset();
}

void Core_Hook::DeinitOther()
{
	if (GetRawInputData) GetRawInputData->Uninstall();
	GetRawInputData.reset();

	if (GetButtonState) GetButtonState->Uninstall();
	GetButtonState.reset();

	if (EngineInitialize) EngineInitialize->Uninstall();
	EngineInitialize.reset();

	if (DestroySubsystems) DestroySubsystems->Uninstall();
	DestroySubsystems.reset();

	WndProc.reset();

	if (Present) Present->Uninstall();
	Present.reset();

	if (ResizeBuffers) ResizeBuffers->Uninstall();
	ResizeBuffers.reset();
}