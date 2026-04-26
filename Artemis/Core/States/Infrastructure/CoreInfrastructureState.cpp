#include "pch.h"
#include "Core/States/Infrastructure/CoreInfrastructureState.h"
#include "Core/States/Infrastructure/Engine/InputState.h"
#include "Core/States/Infrastructure/Engine/LifecycleState.h"
#include "Core/States/Infrastructure/Engine/RenderState.h"
#include "Core/States/Infrastructure/Persistence/SettingsState.h"

CoreInfrastructureState::CoreInfrastructureState()
{
	Input = std::make_unique<InputState>();
	Lifecycle = std::make_unique<LifecycleState>();
	Render = std::make_unique<RenderState>();
	Settings = std::make_unique<SettingsState>();
}

CoreInfrastructureState::~CoreInfrastructureState() = default;