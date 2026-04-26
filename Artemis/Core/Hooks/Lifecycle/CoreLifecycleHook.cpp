#include "pch.h"
#include "Core/Hooks/Lifecycle/CoreLifecycleHook.h"
#include "Core/Hooks/Lifecycle/GameEngineStartHook.h"
#include "Core/Hooks/Lifecycle/EngineInitializeHook.h"
#include "Core/Hooks/Lifecycle/DestroySubsystemsHook.h"

CoreLifecycleHook::CoreLifecycleHook()
{
	GameEngineStart = std::make_unique<GameEngineInitHook>();
	EngineInitialize = std::make_unique<EngineInitializeHook>();
	DestroySubsystems = std::make_unique<DestroySubsystemsHook>();
}

CoreLifecycleHook::~CoreLifecycleHook() = default;