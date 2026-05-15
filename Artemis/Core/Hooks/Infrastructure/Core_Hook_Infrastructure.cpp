#include "pch.h"

// Header.
#include "Core_Hook_Infrastructure.h"

#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Domain/Core_Hook_Domain.h"

#include "Core/States/Core_State.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Interface/Core_System_Interface.h"

#include "Core/UI/Core_UI.h"

// Input.
#include "Core/Hooks/Infrastructure/Input/Hook_GetButtonState.h"
#include "Core/Hooks/Infrastructure/Input/Hook_GetRawInputData.h"

// Lifecycle.
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_GameEngineInit.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_EngineInitialize.h"
#include "Core/Hooks/Infrastructure/Lifecycle/Hook_DestroySubsystems.h"

// Render.
#include "Core/Hooks/Infrastructure/Render/Hook_ResizeBuffers.h"
#include "Core/Hooks/Infrastructure/Render/Hook_Present.h"

// Window.
#include "Core/Hooks/Infrastructure/Window/Hook_WndProc.h"

Core_Hook_Infrastructure::Core_Hook_Infrastructure() = default;
Core_Hook_Infrastructure::~Core_Hook_Infrastructure() = default;

void Core_Hook_Infrastructure::Initialize(Core_State& state,
	Core_System& system, Core_UI& ui, Core_Hook_Domain& hookDomain)
{
	// Input.
	GetButtonState = std::make_unique<Hook_GetButtonState>(
		Hook_GetButtonState_Dependencies {
			.State_Input = *state.Infrastructure->Input,
			.System_AOBScanner = *system.Infrastructure->AOBScanner,
			.System_Debug = *system.Interface->Debug,
		}
	);

	GetRawInputData = std::make_unique<Hook_GetRawInputData>(
		Hook_GetRawInputData_Dependencies{
			.State_Settings = *state.Infrastructure->Settings,
			.System_Debug = *system.Interface->Debug,
		}
	);

	// Lifecycle.
	GameEngineStart = std::make_unique<Hook_GameEngineInit>(
		Hook_GameEngineInit_Dependencies {
			.System_AOBScanner = *system.Infrastructure->AOBScanner,
			.System_Debug = *system.Interface->Debug,
		}
	);

	EngineInitialize = std::make_unique<Hook_EngineInitialize>(
		Hook_EngineInitialize_Dependencies {
			.Hook_BlamOpenMap = *hookDomain.BlamOpenMap,
			.Hook_ObjectTable = *hookDomain.ObjectTable,
			.Hook_CreateObject = *hookDomain.CreateGameObject,
			.Hook_ReleaseObject = *hookDomain.ReleaseGameObject,
			.Hook_PlayerTable = *hookDomain.PlayerTable,
			.Hook_CreatePlayer = *hookDomain.CreatePlayer,
			.Hook_InteractionTable = *hookDomain.InteractionTable,
			.Hook_GetButtonState = *GetButtonState,
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.System_AOBScanner = *system.Infrastructure->AOBScanner,
			.System_Debug = *system.Interface->Debug,
		}
	);

	DestroySubsystems = std::make_unique<Hook_DestroySubsystems>(
		Hook_DestroySubsystems_Dependencies {
			.Hook_BlamOpenMap = *hookDomain.BlamOpenMap,
			.Hook_CreateObject = *hookDomain.CreateGameObject,
			.Hook_ReleaseObject = *hookDomain.ReleaseGameObject,
			.Hook_CreatePlayer = *hookDomain.CreatePlayer,
			.Hook_GetButtonState = *GetButtonState,
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.System_Map = *system.Domain->Map,
			.System_MapTagGroup = *system.Domain->MapTagGroup,
			.System_ObjectTable = *system.Domain->ObjectTable,
			.System_PlayerTable = *system.Domain->PlayerTable,
			.System_InteractionTable = *system.Domain->InteractionTable,
			.System_ObjectClassifier = *system.Domain->ObjectClassifier,
			.System_ObjectGraph = *system.Domain->ObjectGraph,
			.System_PlayerGraph = *system.Domain->PlayerGraph,
			.System_Navigation = *system.Domain->Navigation,
			.System_Environment = *system.Domain->Environment,
			.System_Interactable = *system.Domain->Interactable,
			.System_AOBScanner = *system.Infrastructure->AOBScanner,
			.System_Debug = *system.Interface->Debug,
			.UI_ObjectTable = *ui.ObjectTable,
			.UI_PlayerTable = *ui.PlayerTable,
			.UI_ObjectGraph = *ui.ObjectGraph,
			.UI_Interactable = *ui.Interactable,
			.UI_Map = *ui.Map,
		}
	);

	// Window.
	WndProc = std::make_unique<Hook_WndProc>(
		Hook_WndProc_Dependencies {
			.State_MemoryScanner = *state.Infrastructure->MemoryScanner,
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.State_Settings = *state.Infrastructure->Settings,
			.System_MemoryScanner = *system.Infrastructure->MemoryScanner,
			.System_Lifecycle = *system.Infrastructure->Lifecycle,
			.System_Preferences = *system.Infrastructure->Preferences,
			.System_Debug = *system.Interface->Debug,
		}
	);

	// Render.
	ResizeBuffers = std::make_unique<Hook_ResizeBuffers>(
		Hook_ResizeBuffers_Dependencies{
			.State_Render = *state.Infrastructure->Render,
			.System_Render = *system.Infrastructure->Render,
			.System_Debug = *system.Interface->Debug,
		}
	);

	Present = std::make_unique<Hook_Present>(
		Hook_Present_Depedencies{
			.Hook_GetRawInputData = *GetRawInputData,
			.Hook_WndProc = *WndProc,
			.State_Render = *state.Infrastructure->Render,
			.System_Render = *system.Infrastructure->Render,
			.System_Debug = *system.Interface->Debug,
			.UI_Main = *ui.Main,
		}
	);
}

void Core_Hook_Infrastructure::Shutdown()
{
	if (Present) Present->Uninstall();
	Present.reset();
	WndProc.reset();

	if (ResizeBuffers) ResizeBuffers->Uninstall();
	ResizeBuffers.reset();

	if (DestroySubsystems) DestroySubsystems->Uninstall();
	DestroySubsystems.reset();

	if (EngineInitialize) EngineInitialize->Uninstall();
	EngineInitialize.reset();

	if (GameEngineStart) GameEngineStart->Uninstall();
	GameEngineStart.reset();

	if (GetRawInputData) GetRawInputData->Uninstall();
	GetRawInputData.reset();

	if (GetButtonState) GetButtonState->Uninstall();
	GetButtonState.reset();
}