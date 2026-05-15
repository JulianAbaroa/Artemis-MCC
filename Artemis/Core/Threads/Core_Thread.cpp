#include "pch.h"

// Header.
#include "Core_Thread.h"

#include "Core/Hooks/Core_Hook.h"
#include "Core/Hooks/Infrastructure/Core_Hook_Infrastructure.h"

#include "Core/States/Core_State.h"
#include "Core/States/Domain/Core_State_Domain.h"
#include "Core/States/Infrastructure/Core_State_Infrastructure.h"

#include "Core/Systems/Core_System.h"
#include "Core/Systems/Domain/Core_System_Domain.h"
#include "Core/Systems/Infrastructure/Core_System_Infrastructure.h"
#include "Core/Systems/Interface/Core_System_Interface.h"

#include "Core/Threads/Domain/Thread_Main.h"
#include "Core/Threads/Infrastructure/Thread_Input.h"
#include "Core/Threads/Domain/Thread_AI.h"

Core_Thread::Core_Thread() = default;
Core_Thread::~Core_Thread() = default;

void Core_Thread::Initialize(Core_State& state, Core_System& system,
	Core_Hook& hook)
{
	Main = std::make_unique<Thread_Main>(
		Thread_Main_Dependencies{
			.Hook_EngineInitialize = *hook.Infrastructure->EngineInitialize,
			.Hook_DestroySubsystems = *hook.Infrastructure->DestroySubsystems,
			.Hook_GameEngineInit = *hook.Infrastructure->GameEngineStart,
			.Hook_Present = *hook.Infrastructure->Present,
			.Hook_ResizeBuffers = *hook.Infrastructure->ResizeBuffers,
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.System_Thread = *system.Infrastructure->Thread,
			.System_Debug = *system.Interface->Debug,
		}
	);

	Input = std::make_unique<Thread_Input>(
		Thread_Input_Dependencies {
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.State_Settings = *state.Infrastructure->Settings,
			.System_Input = *system.Infrastructure->Input,
			.System_Thread = *system.Infrastructure->Thread,
			.System_Debug = *system.Interface->Debug,
		}
	);

	AI = std::make_unique<Thread_AI>(
		Thread_AI_Dependencies {
			.State_Map = *state.Domain->Map,
			.State_PlayerTable = *state.Domain->PlayerTable,
			.State_Navigation = *state.Domain->Navigation,
			.State_Lifecycle = *state.Infrastructure->Lifecycle,
			.System_ObjectTable = *system.Domain->ObjectTable,
			.System_PlayerTable = *system.Domain->PlayerTable,
			.System_InteractionTable = *system.Domain->InteractionTable,
			.System_ObjectClassifier = *system.Domain->ObjectClassifier,
			.System_ObjectGraph = *system.Domain->ObjectGraph,
			.System_PlayerGraph = *system.Domain->PlayerGraph,
			.System_Navigation = *system.Domain->Navigation,
			.System_Environment = *system.Domain->Environment,
			.System_Interactable = *system.Domain->Interactable,
			.System_Debug = *system.Interface->Debug,
		}
	);
}

void Core_Thread::Shutdown()
{
	Main.reset();
	Input.reset();
	AI.reset();
}