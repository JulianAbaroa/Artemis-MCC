#include "pch.h"

#include "Core_Thread.h"

#include "Core/Hooks/Core_Hook.h"

#include "Core/States/Core_State.h"

#include "Core/Systems/Core_System.h"

#include "Core/Threads/Main/Thread_Main.h"
#include "Core/Threads/Input/Thread_Input.h"
#include "Core/Threads/AI/Thread_AI.h"

Core_Thread::Core_Thread() = default;
Core_Thread::~Core_Thread() = default;

void Core_Thread::Initialize(Core_State& state, 
	Core_System& system, Core_Hook& hook)
{
	Main = std::make_unique<Thread_Main>(
		Thread_Main_Dependencies{
			.Hook_EngineInitialize = *hook.EngineInitialize,
			.Hook_DestroySubsystems = *hook.DestroySubsystems,
			.Hook_ResizeBuffers = *hook.ResizeBuffers,
			.Hook_Present = *hook.Present,
			.State_Lifecycle = *state.Lifecycle,
			.System_Telemetry = *system.Telemetry,
			.System_Logs = *system.Logs,
		});

	Input = std::make_unique<Thread_Input>(
		Thread_Input_Dependencies {
			.State_Settings = *state.Settings,
			.State_Lifecycle = *state.Lifecycle,
			.System_Input = *system.Input,
			.System_Logs = *system.Logs,
		});

	AI = std::make_unique<Thread_AI>(
		Thread_AI_Deps {
			.State_MapReader = *state.MapReader,
			.State_WorldBuilder = *state.WorldBuilder,
			.State_Lifecycle = *state.Lifecycle,
			.State_Telemetry = *state.Telemetry,
			.System_TagGroupReader = *system.TagGroupReader,
			.System_WorldBuilder = *system.WorldBuilder,
			.System_StatsBuilder = *system.StatsBuilder,
			.System_VitalityBuilder = *system.VitalityBuilder,
			.System_ObjectTable = *system.ObjectTable,
			.System_PlayerTable = *system.PlayerTable,
			.System_InteractionTable = *system.InteractionTable,
			.System_Classifier = *system.Classifier,
			.System_ObjectGraph = *system.ObjectGraph,
			.System_PlayerGraph = *system.PlayerGraph,
			.System_Collidables = *system.Collidables,
			.System_Fixtures = *system.Fixtures,
			.System_Affordances = *system.Affordances,
			.System_Vitality = *system.Vitality,
			.System_Self = *system.Self,
			.System_Tick = *system.Tick,
			.System_Logs = *system.Logs,
		});
}

void Core_Thread::Deinitialize()
{
	Main.reset();
	Input.reset();
	AI.reset();
}