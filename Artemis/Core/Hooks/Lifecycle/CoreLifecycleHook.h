#pragma once

#include <memory>

class GameEngineInitHook;
class EngineInitializeHook;
class DestroySubsystemsHook;

struct CoreLifecycleHook
{
	CoreLifecycleHook();
	~CoreLifecycleHook();

	std::unique_ptr<GameEngineInitHook> GameEngineStart;
	std::unique_ptr<EngineInitializeHook> EngineInitialize;
	std::unique_ptr<DestroySubsystemsHook> DestroySubsystems;
};